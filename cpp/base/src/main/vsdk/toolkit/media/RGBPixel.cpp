#include <cstdio>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/RGBPixel.h"
void RGBPixel::importFromColorRgb(const ColorRgb& c) {
    double ir = c.r();
    double ig = c.g();
    double ib = c.b();

    if (ir < 0.0) ir = 0.0;
    if (ig < 0.0) ig = 0.0;
    if (ib < 0.0) ib = 0.0;
    if (ir > 1.0) ir = 1.0;
    if (ig > 1.0) ig = 1.0;
    if (ib > 1.0) ib = 1.0;

    this->r = (char)((int)(ir * 255.0));
    this->g = (char)((int)(ig * 255.0));
    this->b = (char)((int)(ib * 255.0));
}

java::String* RGBPixel::toString() const {
    char buffer[256];
    int ur = (int)(unsigned char)r;
    int ug = (int)(unsigned char)g;
    int ub = (int)(unsigned char)b;

    snprintf(buffer, sizeof(buffer), "<%d, %d, %d>", ur, ug, ub);
    return new java::String(buffer);
}
