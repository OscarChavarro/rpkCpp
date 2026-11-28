#ifndef __RGB_PRCDRL_CLR_PLTT__
#define __RGB_PRCDRL_CLR_PLTT__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/media/RGBColorPalette.h"
/**
The RGBProceduralColorPalette abstract class provides an interface for
procedural color palette classes. This serves two purposes:
  - To help in design level organization of procedural color palettes
    (this eases the study of the class hierarchy)
  - To provide a place to locate operations and methods, common to all
    procedural color palette classes.

Note that any procedural color palette is a normal color palette. Its
difference is that some operations (most notably the selectNearestIndexToRgb
method) are more rapidly calculated in some procedural defined palettes that
its normal method.
*/
class RGBProceduralColorPalette : public RGBColorPalette {

protected:
    /// A procedural palette is "pure" if its contents are the original ones
    /// defined by its filling procedure method "init", and are not pure
    /// if they are modified in any other way. A pure procedural palette may
    /// have a property that allows for faster calculation of some operation,
    /// whereas a non-pure equivalent will work equally, but slower.
    BOOLEAN pure;

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    RGBProceduralColorPalette();
    virtual ~RGBProceduralColorPalette();

    int selectNearestIndexToRgb(const ColorRgb& c) const;

    virtual void setColorAt(int i, ColorRgb* c);
    virtual void setColorAt(int i, double r, double g, double b);

    virtual void addColor(ColorRgb* c);
    virtual void addColor(double r, double g, double b);

    BOOLEAN isPure() const { return pure; }
};

#endif
