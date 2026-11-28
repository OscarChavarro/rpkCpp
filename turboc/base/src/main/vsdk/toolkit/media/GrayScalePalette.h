#ifndef __GRAY_SCALE_PALETTE__
#define __GRAY_SCALE_PALETTE__

#include "vsdk/toolkit/media/RGBProceduralColorPalette.h"
class ColorRgb;

/**
Represents a linear scale gray palette.
*/
class GrayScalePalette : public RGBProceduralColorPalette {

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    GrayScalePalette();
    virtual ~GrayScalePalette() {}

    int selectNearestIndexToRgb(const ColorRgb& c) const;
};

#endif
