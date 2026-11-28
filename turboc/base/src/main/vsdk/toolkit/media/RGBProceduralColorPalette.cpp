#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/RGBProceduralColorPalette.h"
RGBProceduralColorPalette::RGBProceduralColorPalette() : RGBColorPalette() {
    pure = TRUE;
}

RGBProceduralColorPalette::~RGBProceduralColorPalette() {
}

int RGBProceduralColorPalette::selectNearestIndexToRgb(const ColorRgb& c) const {
    return RGBColorPalette::selectNearestIndexToRgb(c);
}

void RGBProceduralColorPalette::setColorAt(int i, ColorRgb* c) {
    pure = FALSE;
    RGBColorPalette::setColorAt(i, c);
}

void RGBProceduralColorPalette::setColorAt(int i, double r, double g, double b) {
    pure = FALSE;
    RGBColorPalette::setColorAt(i, r, g, b);
}

void RGBProceduralColorPalette::addColor(ColorRgb* c) {
    pure = FALSE;
    RGBColorPalette::addColor(c);
}

void RGBProceduralColorPalette::addColor(double r, double g, double b) {
    pure = FALSE;
    RGBColorPalette::addColor(r, g, b);
}

const Class RGBProceduralColorPalette::CLASS = { "RGBProceduralColorPalette", &RGBColorPalette::CLASS };

const Class *
RGBProceduralColorPalette::getClass() const
{
    return &CLASS;
}
