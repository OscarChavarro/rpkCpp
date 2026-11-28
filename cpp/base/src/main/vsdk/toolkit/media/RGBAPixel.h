#ifndef __RGBA_PIXEL__
#define __RGBA_PIXEL__

#include "java/lang/String.h"
#include "vsdk/toolkit/media/MediaEntity.h"
/**
Respect to data representation:

The `r`, `g`, `b` and `a` class attributes represent red, green, blue and
alpha components in a color specification, with values in the range
[0, 255], for use in color raster systems.

Note that the `r`, `g`, `b` and `a` class attributes are PUBLIC, converting
this class in an not evolvable structure, and IT MUST BE KEEP AS IS, due to
performance issues in a lot of algorithms, as this avoids indirections.
Nevertheless, get and set methods are provided.
*/
class RGBAPixel : public MediaEntity {

public:
    char r;
    char g;
    char b;
    char a;

    RGBAPixel() : r(0), g(0), b(0), a(0) {}
    RGBAPixel(char r_, char g_, char b_, char a_) : r(r_), g(g_), b(b_), a(a_) {}
    RGBAPixel(const RGBAPixel& other) : r(other.r), g(other.g), b(other.b), a(other.a) {}

    ~RGBAPixel() = default;

    void setR(char r_) { this->r = r_; }
    char getR() const { return r; }

    void setG(char g_) { this->g = g_; }
    char getG() const { return g; }

    void setB(char b_) { this->b = b_; }
    char getB() const { return b; }

    void setA(char a_) { this->a = a_; }
    char getA() const { return a; }

    java::String* toString() const;
};

#endif
