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

template <typename T>
Vec3<T> cross(Vec3<T> a, Vec3<T> b)
{
    return Vec3<T>(a.y * b.z - b.y * a.z, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

bool IsInTriangle(Vec2i point, Vec2i* t)
{
    Vec2i AB = t[1] - t[0];
    Vec2i AC = t[2] - t[0];
    Vec2i PA = t[0] - point;

    Vec3i v1{ AB.x, AC.x, PA.x };
    Vec3i v2{ AB.y, AC.y, PA.y };

    int crossZ = (v1.x * v2.y) - (v1.y * v2.x);

    //point and t's has integers as coordinates, so abs(crossZ) < 1 means that crossZ is 0
    //and thus triangle is degenerate
    if (std::abs(crossZ) < 1)
        return false;

    float crossX = (v1.y * v2.z) - (v2.y * v1.z);
    float crossY = (v2.x * v1.z) - (v1.x * v2.z);
    
    Vec3f barycentric{ 1 - (crossX + crossY) / crossZ, crossY / crossZ, crossX / crossZ };
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
        Vec2i pix{ (i % colNum) + bb.lowerLeft.x, (i / colNum) + bb.lowerLeft.y };
        if (IsInTriangle(pix, t))
            image.set(pix.x, pix.y, color);
    }
}

int main(int argc, char** argv)
{
    TGAImage frame(200, 200, TGAImage::RGB);
    Vec2i pts[3] = { Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160) };
    filled_triangle(pts, frame, TGAColor(255, 0, 0, 255));
    frame.flip_vertically(); // to place the origin in the bottom left corner of the image 
    frame.write_tga_file("framebuffer.tga");
    //return 0;

    getchar();

    return 0;
}

