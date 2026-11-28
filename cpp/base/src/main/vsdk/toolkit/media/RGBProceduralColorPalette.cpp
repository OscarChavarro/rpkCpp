#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/RGBProceduralColorPalette.h"
RGBProceduralColorPalette::RGBProceduralColorPalette() : RGBColorPalette() {
    pure = true;
}

RGBProceduralColorPalette::~RGBProceduralColorPalette() {
}

int RGBProceduralColorPalette::selectNearestIndexToRgb(const ColorRgb& c) const {
    return RGBColorPalette::selectNearestIndexToRgb(c);
}

void RGBProceduralColorPalette::setColorAt(int i, ColorRgb* c) {
    pure = false;
    RGBColorPalette::setColorAt(i, c);
}

void RGBProceduralColorPalette::setColorAt(int i, double r, double g, double b) {
    pure = false;
    RGBColorPalette::setColorAt(i, r, g, b);
}

void RGBProceduralColorPalette::addColor(ColorRgb* c) {
    pure = false;
    RGBColorPalette::addColor(c);
}

void RGBProceduralColorPalette::addColor(double r, double g, double b) {
    pure = false;
    RGBColorPalette::addColor(r, g, b);
}
