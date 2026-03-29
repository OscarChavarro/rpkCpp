/**
Definitions for polygon package
*/

#ifndef __POLY_HDR__
#define __POLY_HDR__

#include "SGL/PolygonConstants.h"
#include "SGL/PolygonVertex.h"
#include "SGL/Polygon.h"
#include "SGL/PolygonBox.h"
#include "SGL/Window.h"
#include "SGL/sgl.h"

/**
Mask is an interpolation mask whose kth bit indicates whether the kth
double in a Poly_vert is relevant.
For example, if the valid attributes are sx, sy, and sz, then set
mask = polyMask(offsetof(PolygonVertex, sx)) |
       polyMask(offsetof(PolygonVertex, sy)) |
       polyMask(offsetof(PolygonVertex, sz));
*/

inline constexpr unsigned long
polyMask(std::size_t elementOffset) {
    return 1UL << (elementOffset / sizeof(double));
}

int polyClipToBox(Polygon *p1, const PolygonBox *box);
void polyScanFlat(SGL_CONTEXT *sglContext, Polygon *p, const Window *win);
void polyScanZ(SGL_CONTEXT *sglContext, Polygon *p, const Window *window);

#endif
