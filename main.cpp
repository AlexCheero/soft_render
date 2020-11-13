#include <vector>
#include <cmath>
#include <algorithm>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0, 255, 0, 255);
Model *model = NULL;
const int width  = 200;
const int height = 200;

int getYForX(int x0, int y0, int x1, int y1, int x)
{
    return y0 + (y1 - y0) * (x - x0) / (float)(x1 - x0);
}

int getYForX(Vec2i t0, Vec2i t1, int x)
{
    return getYForX(t0.y, t0.x, t1.y, t1.x, x);
}

int getXForY(Vec2i t0, Vec2i t1, int y)
{
    return getYForX(t0.y, t0.x, t1.y, t1.x, y);
}

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
    bool steep = false;
    if (std::abs(x0-x1)<std::abs(y0-y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0>x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    for (int x = x0; x <= x1; x++)
    {
        int y = getYForX(x0, y0, x1, y1, x);
        if (steep)
            image.set(y, x, color);
        else
            image.set(x, y, color);
    }
}

void line(Vec2i t0, Vec2i t1, TGAImage& image, TGAColor color)
{
    line(t0.x, t0.y, t1.x, t1.y, image, color);
}

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color)
{
    line(t0, t1, image, color);
    line(t1, t2, image, color);
    line(t2, t0, image, color);
}

/*
void filled_triangle(Vec2i* t, TGAImage& image, TGAColor color)
{
    std::sort(t, t + 3, [](Vec2i t1, Vec2i t2) { return t1.y < t2.y; });
    for (int i = t[0].y; i < t[1].y + 1; i++)
    {
        int x1 = getXForY(t[0], t[1], i);
        int x2 = getXForY(t[0], t[2], i);

        line(x1, i, x2, i, image, color);
    }

    for (int i = t[2].y; i > t[1].y; i--)
    {
        int x1 = getXForY(t[2], t[1], i);
        int x2 = getXForY(t[2], t[0], i);

        line(x1, i, x2, i, image, color);
    }
}
*/

bool IsInTriangle(Vec2i point, Vec2i* t)
{
    Vec2i AB = t[1] - t[0];
    Vec2i AC = t[2] - t[0];
    Vec2i PA = t[0] - point;

    Vec3i v1{ AB.x, AC.x, PA.x };
    Vec3i v2{ AB.y, AC.y, PA.y };

    int crossZ = (v1.x * v2.y) - (v1.y * v2.x);

    if (crossZ < 1)
        return false;

    int crossX = (v1.y * v2.z) - (v2.y * v1.z);
    int crossY = (v2.x * v1.z) - (v1.x * v2.z);
    
    Vec3i barycentric{ 1 - (crossX + crossY) / crossZ, crossY / crossZ, crossX / crossZ };
    //Vec3i barycentric{ 1 - (crossX + crossY), crossX, crossY };
    return barycentric.x >= 0 && barycentric.y >= 0 && barycentric.z >= 0;
}

struct BoundingBox
{
    Vec2i lowerLeft;
    Vec2i upperRight;
};

void filled_triangle(Vec2i* t, TGAImage& image, TGAColor color)
{
    BoundingBox bb{ t[0], t[0] };
    for (int i = 0; i < 3; i++)
    {
        Vec2i vert = t[i];
        if (bb.lowerLeft.x > vert.x)
            bb.lowerLeft.x = vert.x;
        if (bb.lowerLeft.y > vert.y)
            bb.lowerLeft.y = vert.y;
        if (bb.upperRight.x < vert.x)
            bb.upperRight.x = vert.x;
        if (bb.upperRight.y < vert.y)
            bb.upperRight.y = vert.y;
    }

    int colNum = bb.upperRight.x - bb.lowerLeft.x;
    int rowNum = bb.upperRight.y - bb.lowerLeft.y;
    int pixNum = colNum * rowNum;

    for (int i = 0; i < pixNum; i++)
    {
        Vec2i pix{ (i % colNum) + bb.lowerLeft.x, (i / rowNum) + bb.lowerLeft.y };
        if (IsInTriangle(pix, t))
            image.set(pix.x, pix.y, color);
    }
}

template <typename T>
Vec3<T> cross(Vec3<T> a, Vec3<T> b)
{
    return Vec3<T>(a.y * b.z - b.y * a.z, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

Vec3f barycentric1(Vec2i* pts, Vec2i P) {
    Vec3f u = cross(Vec3f(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0]), Vec3f(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]));
    /* `pts` and `P` has integer value as coordinates
       so `abs(u[2])` < 1 means `u[2]` is 0, that means
       triangle is degenerate, in this case return something with negative coordinates */
    if (std::abs(u[2]) < 1) return Vec3f(-1, 1, 1);
    return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

void filled_triangle1(Vec2i* pts, TGAImage& image, TGAColor color) {
    Vec2i bboxmin(image.get_width() - 1, image.get_height() - 1);
    Vec2i bboxmax(0, 0);
    Vec2i clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    Vec2i P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric1(pts, P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            image.set(P.x, P.y, color);
        }
    }
}

int main(int argc, char** argv)
{
    TGAImage frame(200, 200, TGAImage::RGB);
    Vec2i pts[3] = { Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160) };
    filled_triangle(pts, frame, TGAColor(255, 0, 0, 255));
    frame.flip_vertically(); // to place the origin in the bottom left corner of the image 
    frame.write_tga_file("framebuffer.tga");
    return 0;

    getchar();

    return 0;
}

