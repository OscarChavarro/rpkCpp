#ifndef __CONVEX_WINDOW_CLIPPER__
#define __CONVEX_WINDOW_CLIPPER__

#include "vsdk/toolkit/environment/geometry/geometricProcessing/convexPolygonClipper/HomogeneousHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/convexPolygonClipper/ScreenPolygon.h"

/**
Clips a convex polygon in homogeneous coordinates against a convex window,
given as an intersection of half-spaces, after [SUTH1974b]. This layer knows nothing of clip boxes, trivial acceptance or
rasterization: those belong to the render/raster/polygonScanConverter package,
which builds on it.
It keeps no state: every call returns a new polygon.
*/
class ConvexWindowClipper final {
  private:
    static double signedMeasure(const PolygonVertex &vertex, const HomogeneousHalfSpace &halfSpace);
    static void checkRoomForVertex(int numberOfVertices);

  public:
    ConvexWindowClipper() = delete;

    static ScreenPolygon clipToHalfSpace(const ScreenPolygon &polygon, const HomogeneousHalfSpace &halfSpace);
    static ScreenPolygon clipToHalfSpaces(const ScreenPolygon &polygon, const HomogeneousHalfSpace *halfSpaces, int numberOfHalfSpaces);
};

#endif
