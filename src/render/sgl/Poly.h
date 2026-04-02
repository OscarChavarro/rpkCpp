/**
Definitions for polygon package
*/

#ifndef __POLY_HDR__
#define __POLY_HDR__

#include <cstddef>

#include "render/sgl/PolygonClipResult.h"
#include "render/sgl/PolygonVertex.h"
#include "render/sgl/Polygon.h"
#include "render/sgl/PolygonBox.h"
#include "render/sgl/Window.h"
#include "render/sgl/SglContext.h"
/**
Mask is an interpolation mask whose kth bit indicates whether the kth
double in a Poly_vert is relevant.
For example, if the valid attributes are sx, sy, and sz, then set
mask = Poly::mask(offsetof(PolygonVertex, sx)) |
       Poly::mask(offsetof(PolygonVertex, sy)) |
       Poly::mask(offsetof(PolygonVertex, sz));
*/

class Poly {
  private:
    static void polygonSwap(Polygon *a, Polygon *b);
    static void setPolygonVertexCoord(PolygonVertex *vertex, int index, double value);
    static void clipToHalfSpace(Polygon *p, Polygon *q, int index, double sign, double k);
    static void clipAndSwap(int elementIndex, double sign, double k, Polygon *p, Polygon *q, Polygon *p1);

    static void incrementalizeYFlat(const PolygonVertex &p1, const PolygonVertex &p2, PolygonVertex *p, PolygonVertex *dp, int y);
    static void incrementFlat(PolygonVertex *p, const PolygonVertex &dp);
    static void scanlineFlat(const SglContext *sglContext, int y, const PolygonVertex *l, const PolygonVertex *r, const Window *win);

    static void incrementalizeYZ(const PolygonVertex &p1, const PolygonVertex &p2, PolygonVertex *p, PolygonVertex *dp, int y);
    static void incrementZ(PolygonVertex *p, const PolygonVertex &dp);
    static void scanlineZ(const SglContext *sglContext, int y, const PolygonVertex *l, const PolygonVertex *r, const Window *win);

  public:
    static constexpr unsigned long
    mask(std::size_t elementOffset) {
        return 1UL << (elementOffset / sizeof(double));
    }

    static int clipToBox(Polygon *p1, const PolygonBox *box);
    static void scanFlat(SglContext *sglContext, Polygon *p, const Window *win);
    static void scanZ(SglContext *sglContext, Polygon *p, const Window *window);
};

#endif
