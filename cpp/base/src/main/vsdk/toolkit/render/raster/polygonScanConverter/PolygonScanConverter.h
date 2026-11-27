#ifndef __POLYGON_SCAN_CONVERTER__
#define __POLYGON_SCAN_CONVERTER__

#include "vsdk/toolkit/environment/geometry/geometricProcessing/convexPolygonClipper/ScreenPolygon.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/RasterRectangle2D.h"

using RasterDepthValue = unsigned long;

/**
Point-sampled scan conversion of convex polygons after [HECK1990], writing one
payload per covered sample, with or without a depth test. It keeps no state. The template
implementations are in PolygonScanFlat.txx and PolygonScanZ.txx.
*/
class PolygonScanConverter final {
  private:
    static void incrementalizeYFlat(const PolygonVertex &p1, const PolygonVertex &p2, int y, double &sx, double &dsx);
    template<class Sample>
    static void scanlineFlat(Sample *samples, int stride, Sample value,
                             int y, double lSx, double rSx, const RasterRectangle2D *win);

    static RasterDepthValue depthValue(double depth);
    static void incrementalizeYZ(const PolygonVertex &p1, const PolygonVertex &p2, int y,
                                 double &sx, double &dsx, double &sz, double &dsz);
    template<class Sample>
    static void scanlineZ(Sample *samples, RasterDepthValue *depth, int stride, Sample value,
                          int y, double lSx, double lSz, double rSx, double rSz, const RasterRectangle2D *win);

  public:
    static constexpr RasterDepthValue MAXIMUM_DEPTH = 4294967295UL;

    PolygonScanConverter() = delete;

    template<class Sample>
    static void scanFlat(Sample *samples, int stride, Sample value, const ScreenPolygon *p, const RasterRectangle2D *win);

    template<class Sample>
    static void scanZ(Sample *samples, RasterDepthValue *depth, int stride, Sample value,
                      const ScreenPolygon *p, const RasterRectangle2D *window);
};

#endif
