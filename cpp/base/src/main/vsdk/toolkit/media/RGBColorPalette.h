#ifndef __RGB_COLOR_PALETTE__
#define __RGB_COLOR_PALETTE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/media/MediaEntity.h"
class ColorRgb;

/**
This class represents a color palette, as an indexed set of colors.
*/
class RGBColorPalette : public MediaEntity {

protected:
    java::ArrayList<ColorRgb*> colors;

public:
    RGBColorPalette();
    virtual ~RGBColorPalette();

    void init(int size);

    int size() const;

    void buildGrayLevelsTable();

    ColorRgb* getColorAt(int i) const;

    virtual void setColorAt(int i, ColorRgb* c);
    virtual void setColorAt(int i, double r, double g, double b);

    virtual void addColor(ColorRgb* c);
    virtual void addColor(double r, double g, double b);

    ColorRgb* evalNearest(double t) const;

    ColorRgb* evalLinear(double t) const;

    int selectNearestIndexToRgb(const ColorRgb& c) const;
};

#endif
