#include <cassert>

#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"

static double one(double, double) { return 1.0; }
static double quarter(double, double) { return 0.25; }
static double negativeHalf(double, double) { return -0.5; }

int main() {
    double (*functions[])(double, double) = {one, quarter, negativeHalf};
    GalerkinBasis basis = {"contract", 3, functions, functions, nullptr};
    const ColorRgbMutable coefficients[] = {
        ColorRgbMutable(1, 2, 3), ColorRgbMutable(4, 8, 12), ColorRgbMutable(4, 2, 8)
    };
    const ColorRgb result = Basismcrad::colorAtUv(&basis, coefficients, 0.2, 0.7);
    assert(result.getR() == 0.0 && result.getG() == 3.0 && result.getB() == 2.0);
    basis.size = 0;
    const ColorRgb empty = Basismcrad::colorAtUv(&basis, coefficients, 0.2, 0.7);
    assert(empty.getR() == 0.0 && empty.getG() == 0.0 && empty.getB() == 0.0);
    return 0;
}
