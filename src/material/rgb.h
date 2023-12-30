/* rgb.h: representation of RGB color triples that can be 
 * displayed on the screen */

#ifndef _RPK_RGB_H_
#define _RPK_RGB_H_

#include "common/mymath.h"
#include "common/linealAlgebra/Float.h"

class RGB {
  public:
    float r;
    float g;
    float b;
};

/* some RGB colors for e.g. debugging purposes */
extern RGB Black, White, Green, Yellow, Red, Magenta, Blue, Turquoise;

inline int operator==(RGB rgb1, RGB rgb2)
{
    return(FLOATEQUAL(rgb1.r, rgb2.r, EPSILON) &&
       FLOATEQUAL(rgb1.g, rgb2.g, EPSILON) &&
       FLOATEQUAL(rgb1.b, rgb2.b, EPSILON));
}

/* macro to print an RGB triplet */
#define RGBPrint(fp, color)    fprintf(fp, "%g %g %g", (color).r, (color).g, (color).b);

/* macro to fill in an RGB structure with given red,green and blue components */
#define RGBSET(color, R, G, B)  ((color).r = (R), (color).g = (G), (color).b = (B))

/* clips RGB intensities to the interval [0,1] */
#define RGBCLIP(color)        (\
            (color).r = ((color).r < 0. ? 0. : ((color).r > 1. ? 1. : (color).r)), \
            (color).g = ((color).g < 0. ? 0. : ((color).g > 1. ? 1. : (color).g)), \
            (color).b = ((color).b < 0. ? 0. : ((color).b > 1. ? 1. : (color).b)))

/* maximum component */
#define RGBMAXCOMPONENT(_r) \
    (_r).r > (_r).g ? MAX((_r).r,(_r).b) : MAX((_r).g,(_r).b)

#endif
