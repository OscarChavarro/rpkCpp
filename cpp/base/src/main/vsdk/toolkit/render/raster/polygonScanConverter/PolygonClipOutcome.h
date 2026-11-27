#ifndef __POLYGON_CLIP_OUTCOME__
#define __POLYGON_CLIP_OUTCOME__

#include "vsdk/toolkit/environment/geometry/geometricProcessing/convexPolygonClipper/ScreenPolygon.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonClipResult.h"

/** Immutable result of a box clip: its classification and the clipped polygon */
class PolygonClipOutcome final {
  private:
    PolygonClipResult result;
    ScreenPolygon polygon;

  public:
    PolygonClipOutcome(PolygonClipResult inResult, const ScreenPolygon &inPolygon):
        result(inResult), polygon(inPolygon) {}

    PolygonClipResult getResult() const { return result; }
    const ScreenPolygon &getPolygon() const { return polygon; }
};

#endif
