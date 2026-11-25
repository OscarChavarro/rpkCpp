#include <cassert>
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
static void check(const ColorRgb &c, double r, double g, double b) {
    assert(c.getR() == r && c.getG() == g && c.getB() == b);
}
int main() {
    check(ColorRgb(2, 4, 6).scaleInverse(0.0), 2, 4, 6);
    check(ColorRgb(2, 4, 6).divide(ColorRgb(0, 2, 3)), 2, 2, 2);
    check(ColorRgb(2, -1, 6).maximum(ColorRgb(1, 4, 7)), 2, 4, 7);
    assert(ColorRgb(2, -1, 8).average() == 3.0);
    Camera camera;
    camera.xSize = 2; camera.ySize = 2;
    camera.pixelWidth = 1.0f; camera.pixelHeight = 1.0f;
    ScreenBuffer a(&camera, &camera), b(&camera, &camera), merged(&camera, &camera);
    a.set(0, 0, ColorRgb(1, 2, 3));
    a.setAddScaleFactor(0.5f);
    a.add(0, 0, ColorRgb(2, 4, 6));
    check(a.get(0, 0), 2, 4, 6);
    a.set(1, 1, ColorRgb(6, 8, 10));
    check(a.get(1, 1), 3, 4, 5);
    check(a.getBiLinear(-0.25f, -0.25f), 1.3125, 2.5, 3.6875);
    b.copy(&a, &camera);
    check(b.get(0, 0), 2, 4, 6);
    b.scaleRadiance(2.0f);
    check(b.get(1, 1), 6, 8, 10);
    merged.merge(&a, &b, &camera);
    check(merged.get(0, 0), 6, 12, 18);
    check(merged.get(1, 1), 9, 12, 15);
    return 0;
}
