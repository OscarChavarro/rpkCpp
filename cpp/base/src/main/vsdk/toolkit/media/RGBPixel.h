#ifndef __RGB_PIXEL__
#define __RGB_PIXEL__

#include "java/lang/String.h"
#include "vsdk/toolkit/media/MediaEntity.h"
/**
Respect to data representation:

The `r`, `g`, and `b` class attributes represent red, green and blue
components in a color specification, with values in the range
[0, 255], for use in color raster systems.

Note that the `r`, `g` and `b` class attributes are PUBLIC, converting
this class in an not evolvable structure, and IT MUST BE KEEP AS IS, due to
performance issues in a lot of algorithms, as this avoids indirections.
Nevertheless, get and set methods are provided.
*/
class RGBPixel : public MediaEntity {

public:
    /// The red component of this RGBPixel
    char r;

    /// The green component of this RGBPixel
    char g;

    /// The blue component of this RGBPixel
    char b;

    RGBPixel() : r(0), g(0), b(0) {}
    RGBPixel(char r_, char g_, char b_) : r(r_), g(g_), b(b_) {}
    RGBPixel(const RGBPixel& other) : r(other.r), g(other.g), b(other.b) {}

    ~RGBPixel() = default;

    void setR(char r_) { this->r = r_; }
    char getR() const { return r; }

    void setG(char g_) { this->g = g_; }
    char getG() const { return g; }

    void setB(char b_) { this->b = b_; }
    char getB() const { return b; }

    void importFromColorRgb(const class ColorRgb& c);

    bool isBlack() const {
        return r == 0 && g == 0 && b == 0;
    }

    java::String* toString() const;
};

#endif
