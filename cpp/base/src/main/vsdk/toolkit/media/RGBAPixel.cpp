#include <cstdio>

#include "java/lang/String.h"
#include "vsdk/toolkit/media/RGBAPixel.h"
java::String* RGBAPixel::toString() const {
    char buffer[256];
    int ur = (int)(unsigned char)r;
    int ug = (int)(unsigned char)g;
    int ub = (int)(unsigned char)b;
    int ua = (int)(unsigned char)a;

    snprintf(buffer, sizeof(buffer), "<%d, %d, %d / (%d)>", ur, ug, ub, ua);
    return new java::String(buffer);
}
