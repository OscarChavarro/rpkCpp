#include <stdio.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/media/RGBAPixel.h"
String* RGBAPixel::toString() const {
    char buffer[256];
    int ur = (int)(unsigned char)r;
    int ug = (int)(unsigned char)g;
    int ub = (int)(unsigned char)b;
    int ua = (int)(unsigned char)a;

    snprintf(buffer, sizeof(buffer), "<%d, %d, %d / (%d)>", ur, ug, ub, ua);
    return new String(buffer);
}

const Class RGBAPixel::CLASS = { "RGBAPixel", &MediaEntity::CLASS };

const Class *
RGBAPixel::getClass() const
{
    return &CLASS;
}
