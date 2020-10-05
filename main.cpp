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

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }
    
    TGAImage image(width, height, TGAImage::RGB);
    
    /*
    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        for (int j=0; j<3; j++) {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j+1)%3]);
            int x0 = (v0.x+1.f)*width/2.;
            int y0 = (v0.y+1.f)*height/2.;
            int x1 = (v1.x+1.f)*width/2.;
            int y1 = (v1.y+1.f)*height/2.;
            line(x0, y0, x1, y1, image, white);
        }
    }
    */

    Vec2i t0[3] = { Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80) };
    Vec2i t1[3] = { Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180) };
    Vec2i t2[3] = { Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180) };
    //triangle(t0[0], t0[1], t0[2], image, red);
    //triangle(t1[0], t1[1], t1[2], image, white);
    //triangle(t2[0], t2[1], t2[2], image, green);

    filled_triangle(t0, image, red);
    filled_triangle(t1, image, white);
    filled_triangle(t2, image, green);

    //line2(t2[0], t2[1], image, red);
    //line2(t2[1], t2[2], image, white);
    //line2(t2[2], t2[0], image, green);

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}

