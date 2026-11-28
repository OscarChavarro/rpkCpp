#ifndef __GRAY_SCALE_PALETTE__
#define __GRAY_SCALE_PALETTE__

#include "vsdk/toolkit/media/RGBProceduralColorPalette.h"
class ColorRgb;

/**
Represents a linear scale gray palette.
*/
class GrayScalePalette : public RGBProceduralColorPalette {

public:
    GrayScalePalette();
    virtual ~GrayScalePalette() = default;

    int selectNearestIndexToRgb(const ColorRgb& c) const;
};

#endif
