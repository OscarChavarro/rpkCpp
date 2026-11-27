#ifndef __POLYGON_BOX_CLIPPER__
#define __POLYGON_BOX_CLIPPER__

#include "vsdk/toolkit/environment/geometry/geometricProcessing/convexPolygonClipper/ScreenPolygon.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonBox.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonClipOutcome.h"

/**
Clips a convex polygon to a screen space box with trivial acceptance and
rejection, after [HECK1990]. The plane by plane clipping is delegated to
ConvexWindowClipper. It keeps no state.
*/
class PolygonBoxClipper final {
  public:
    PolygonBoxClipper() = delete;

    static PolygonClipOutcome clipToBox(const ScreenPolygon &polygon, const PolygonBox &box);
};

#endif
