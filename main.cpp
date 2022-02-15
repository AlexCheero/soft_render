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
const int width  = 800;
const int height = 800;
const int pixCount = width * height;
float zBuffer[pixCount];

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

Vec3f GetBarycentric(Vec3f point, Vec3f* t)
{
    Vec2f AB = Vec2f(t[1].x, t[1].y) - Vec2f(t[0].x, t[0].y);
    Vec2f AC = Vec2f(t[2].x, t[2].y) - Vec2f(t[0].x, t[0].y);
    Vec2f PA = Vec2f(t[0].x, t[0].y) - Vec2f(point.x, point.y);

    Vec3f v1{ AB.x, AC.x, PA.x };
    Vec3f v2{ AB.y, AC.y, PA.y };

    int crossZ = (v1.x * v2.y) - (v1.y * v2.x);

    //point and t's has integers as coordinates, so abs(crossZ) < 1 means that crossZ is 0
    //and thus triangle is degenerate
    if (std::abs(crossZ) < 1)
        return Vec3f(-1);

    float crossX = (v1.y * v2.z) - (v2.y * v1.z);
    float crossY = (v2.x * v1.z) - (v1.x * v2.z);

    return Vec3f{ 1 - (crossX + crossY) / crossZ, crossX / crossZ, crossY / crossZ };
}

bool IsInTriangle(Vec3f barycentric)
{
    return barycentric.x >= 0 && barycentric.y >= 0 && barycentric.z >= 0;
}

struct BoundingBox
{
    Vec2f lowerLeft;
    Vec2f upperRight;
};

void filled_triangle(Vec3f* t, TGAImage& image, Vec2i* uvs, Model* model, float zBuffer[])
{
    BoundingBox bb{ Vec2f(std::numeric_limits<float>::max()), Vec2f(-std::numeric_limits<float>::max()) };
    for (int i = 0; i < 3; i++)
    {
        Vec3f vert = t[i];
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
        Vec3f pix{ (i % colNum) + bb.lowerLeft.x, (i / colNum) + bb.lowerLeft.y, 0/*sum of top's z times corresponding barycentric coord*/ };
        Vec3f barycentric = GetBarycentric(pix, t);
        for (int j = 0; j < 3; j++)
            pix.z += barycentric[j] * t[j].z;
        if (barycentric.x < 0 || barycentric.y < 0 || barycentric.z < 0)
            continue;

        if (!IsInTriangle(barycentric))
            continue;

        int zBufferIndex = pix.x + width * pix.y;
        if (zBuffer[zBufferIndex] >= pix.z)
            continue;

        Vec2i uv;
        uv += uvs[0] * barycentric[0];
        uv += uvs[1] * barycentric[1];
        uv += uvs[2] * barycentric[2];

        zBuffer[zBufferIndex] = pix.z;
        image.set(pix.x, pix.y, model->diffuse(uv));
    }
}

Vec3f world2screen(Vec3f v)
{
    return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}

int main(int argc, char** argv)
{
    TGAImage frame(width, height, TGAImage::RGB);

    for (int i = 0; i < pixCount; i++)
        zBuffer[i] = -std::numeric_limits<float>::max();
    
    model = new Model("obj/african_head.obj");

    const Vec3f light_dir(0, 0, -1);
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        for (int j = 0; j < 3; j++) {
            world_coords[j] = model->vert(face[j]);
            screen_coords[j] = world2screen(world_coords[j]);
        }
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0)
        {
            Vec2i uvs[3];
            for (int j = 0; j < 3; j++)
                uvs[j] = model->uv(i, j);
            filled_triangle(screen_coords, frame, uvs, model, zBuffer);
        }
    }

    frame.flip_vertically(); // to place the origin in the bottom left corner of the image 
    frame.write_tga_file("framebuffer.tga");

    delete model;

    getchar();

    return 0;
}

