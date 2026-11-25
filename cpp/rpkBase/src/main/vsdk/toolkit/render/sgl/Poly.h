/**
Definitions for polygon package
*/

#ifndef __POLY__
#define __POLY__

#include <cstddef>

#include "vsdk/toolkit/render/sgl/PolygonClipResult.h"
#include "vsdk/toolkit/render/sgl/PolygonVertex.h"
#include "vsdk/toolkit/render/sgl/ScreenPolygon.h"
#include "vsdk/toolkit/render/sgl/PolygonBox.h"
#include "vsdk/toolkit/render/sgl/Window.h"
#include "vsdk/toolkit/render/sgl/SglConstants.h"
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
    static void polygonSwap(ScreenPolygon *a, ScreenPolygon *b);
    static void setPolygonVertexCoord(PolygonVertex *vertex, int index, double value);
    static void clipToHalfSpace(ScreenPolygon *p, ScreenPolygon *q, int index, double sign, double k);
    static void clipAndSwap(int elementIndex, double sign, double k, ScreenPolygon *p, ScreenPolygon *q, ScreenPolygon *p1);

    static void incrementalizeYFlat(const PolygonVertex &p1, const PolygonVertex &p2, PolygonVertex *p, PolygonVertex *dp, int y);
    static void incrementFlat(PolygonVertex *p, const PolygonVertex &dp);
    template<class Sample>
    static void scanlineFlat(Sample *samples, int stride, Sample value,
                             int y, const PolygonVertex *l, const PolygonVertex *r, const Window *win);

    static void incrementalizeYZ(const PolygonVertex &p1, const PolygonVertex &p2, PolygonVertex *p, PolygonVertex *dp, int y);
    static void incrementZ(PolygonVertex *p, const PolygonVertex &dp);
    template<class Sample>
    static void scanlineZ(Sample *samples, SglDepthValue *depth, int stride, Sample value,
                          int y, const PolygonVertex *l, const PolygonVertex *r, const Window *win);

  public:
    static constexpr unsigned long
    mask(std::size_t elementOffset) {
        return 1UL << (elementOffset / sizeof(double));
    }

    static int clipToBox(ScreenPolygon *p1, const PolygonBox *box);
    template<class Sample>
    static void scanFlat(Sample *samples, int stride, Sample value, const ScreenPolygon *p, const Window *win);

    template<class Sample>
    static void scanZ(Sample *samples, SglDepthValue *depth, int stride, Sample value,
                      const ScreenPolygon *p, const Window *window);
};

#endif
