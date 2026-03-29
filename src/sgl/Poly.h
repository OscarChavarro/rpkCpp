/**
Definitions for polygon package
*/

#ifndef __POLY_HDR__
#define __POLY_HDR__

#include "sgl/PolygonClipResult.h"
#include "sgl/PolygonVertex.h"
#include "sgl/Polygon.h"
#include "sgl/PolygonBox.h"
#include "sgl/Window.h"
#include "sgl/SglContext.h"
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
void polyScanFlat(SglContext *sglContext, Polygon *p, const Window *win);
void polyScanZ(SglContext *sglContext, Polygon *p, const Window *window);

#endif
