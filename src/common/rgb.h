/**
Representation of RGB color triples that can be displayed on the screen
*/

#ifndef _RPK_RGB_H_
#define _RPK_RGB_H_

#include <cstdio>

#include "common/mymath.h"
#include "common/linealAlgebra/Float.h"

class RGB {
  public:
    float r;
    float g;
    float b;

    /**
    Clips RGB intensities to the interval [0,1]
    */
    inline void
    clip() {
        r = (r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r));
        g = (g < 0.0f ? 0.0f : (g > 1.0f ? 1.0f : g));
        b = (b < 0.0f ? 0.0f : (b > 1.0f ? 1.0f : b));
    }
};

extern RGB GLOBAL_material_black;
extern RGB GLOBAL_material_white;
extern RGB GLOBAL_material_yellow;

inline int operator==(RGB rgb1, RGB rgb2)
{
    return(doubleEqual(rgb1.r, rgb2.r, EPSILON) &&
           doubleEqual(rgb1.g, rgb2.g, EPSILON) &&
           doubleEqual(rgb1.b, rgb2.b, EPSILON));
}

/**
printRegularHierarchy an RGB triplet
*/
inline void
printRGB(FILE *fp, RGB color) {
    fprintf(fp, "%g %g %g", color.r, color.g, color.b);
}

/**
Fill in an RGB structure with given red, green and blue components
*/
inline void
setRGB(RGB &color, float R, float G, float B) {
    color.r = R;
    color.g = G;
    color.b = B;
}

/**
Maximum component
*/
inline float
RGBMAXCOMPONENT(RGB &color) {
    return color.r > color.g ? floatMax(color.r, color.b) : floatMax(color.g, color.b);
}

#endif
