/**
Definitions for polygon package
*/

#ifndef __POLY_HDR__
#define __POLY_HDR__

#include "common/mymath.h"
#include "SGL/sgl.h"

// Note that poly_clip, given an n-gon as input, might output an (n+6)gon
#define MAXIMUM_SIDES_PER_POLYGON 10

class PolygonVertex {
  public:
    double sx; // Screen space position (sometimes homo)
    double sy;
    double sz;
    double sw;
    double x; // World space position
    double y;
    double z;
    double u; // Texture position (sometimes homogeneous)
    double v;
    //double q;
    double r; // (red,green,blue) color
    double g;
    double b;

    inline double
    getCoord(int i) const {
        switch ( i ) {
            case 0:
                return sx;
            case 1:
                return sy;
            case 2:
                return sz;
            case 3:
                return sw;
            case 4:
                return x;
            case 5:
                return y;
            case 6:
                return z;
            case 7:
                return u;
            case 8:
                return v;
            case 9:
                return r;
            case 10:
                return g;
            case 11:
                return b;
            default:
                return 0.0;
        }
    }
};

// Note: don't put > 32 doubles in Poly_vert, or mask will overflow

class Polygon {
  public:
    int n; // Number of sides
    unsigned long mask; // Interpolation mask for vertex elems
    PolygonVertex vertices[MAXIMUM_SIDES_PER_POLYGON];
};

/**
Mask is an interpolation mask whose kth bit indicates whether the kth
double in a Poly_vert is relevant.
For example, if the valid attributes are sx, sy, and sz, then set
mask = POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz);
*/

// A BOX (TYPICALLY IN SCREEN SPACE)
class PolygonBox {
  public:
    double x0; // Left and right
    double x1;
    double y0; // Top and bottom
    double y1;
    double z0; // Near and far
    double z1;
};

// WINDOW: A DISCRETE 2-D RECTANGLE
class Window {
  public:
    int x0; // x-min and y-min
    int y0;
    int x1; // x-max and y-max (inclusive)
    int y1;
};

#define POLY_MASK(elem) (1 << (&GLOBAL_sgl_polyDummy->elem - (double *)GLOBAL_sgl_polyDummy))

// Polygon entirely outside box
#define POLY_CLIP_OUT 0

// Polygon partially inside
#define POLY_CLIP_PARTIAL 1

// Polygon entirely inside box
#define POLY_CLIP_IN 2

// Used superficially by POLY_MASK macro
extern PolygonVertex *GLOBAL_sgl_polyDummy;

int polyClipToBox(Polygon *p1, const PolygonBox *box);
void polyClipToHalfSpace(Polygon *p, Polygon *q, int index, double sign, double k);
void polyScanFlat(SGL_CONTEXT *sglContext, Polygon *p, const Window *win);
void polyScanZ(SGL_CONTEXT *sglContext, Polygon *p, const Window *window);

#endif
