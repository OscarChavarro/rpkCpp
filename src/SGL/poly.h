/* poly.h: definitions for polygon package */

#ifndef POLY_HDR
#define POLY_HDR

#include "common/mymath.h"

#define POLY_NMAX 10        /* max #sides to a polygon; change if needed */
/* note that poly_clip, given an n-gon as input, might output an (n+6)gon */
/* POLY_NMAX=10 is thus appropriate if input polygons are triangles or quads */

class Poly_vert {        /* A POLYGON Vertex */
  public:
    double sx, sy, sz, sw;    /* screen space position (sometimes homo.) */
    double x, y, z;        /* world space position */
    double u, v, q;        /* texture position (sometimes homogeneous) */
    double r, g, b;        /* (red,green,blue) color */
    double nx, ny, nz;        /* world space normal vector */
};

/* note: don't put > 32 doubles in Poly_vert, or mask will overflow */

class Poly {        /* A POLYGON */
  public:
    int n;            /* number of sides */
    unsigned long mask;        /* interpolation mask for vertex elems */
    Poly_vert vert[POLY_NMAX];    /* vertices */
};
/*
 * mask is an interpolation mask whose kth bit indicates whether the kth
 * double in a Poly_vert is relevant.
 * For example, if the valid attributes are sx, sy, and sz, then set
 *	mask = POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz);
 */

class Poly_box {        /* A BOX (TYPICALLY IN SCREEN SPACE) */
  public:
    double x0, x1;        /* left and right */
    double y0, y1;        /* top and bottom */
    double z0, z1;        /* near and far */
};

class Window {        /* WINDOW: A DISCRETE 2-D RECTANGLE */
  public:
    int x0, y0;            /* xmin and ymin */
    int x1, y1;            /* xmax and ymax (inclusive) */
};

#define POLY_MASK(elem) (1 << (&poly_dummy->elem - (double *)poly_dummy))

#define POLY_CLIP_OUT 0        /* polygon entirely outside box */
#define POLY_CLIP_PARTIAL 1    /* polygon partially inside */
#define POLY_CLIP_IN 2        /* polygon entirely inside box */

extern Poly_vert *poly_dummy;    /* used superficially by POLY_MASK macro */

int poly_clip_to_box(Poly *p1, Poly_box *box);

void poly_clip_to_halfspace(Poly *p, Poly *q, int index, double sign, double k);

/* optimized versions for flat shading without and with Z buffering. */
void poly_scan_flat(Poly *p, Window *win);

void poly_scan_z(Poly *p, Window *win);

#endif
