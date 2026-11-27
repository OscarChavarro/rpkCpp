#ifndef __POLYGON_SCAN_CONVERTER__
#define __POLYGON_SCAN_CONVERTER__

#include "vsdk/toolkit/environment/geometry/geometricProcessing/convexPolygonClipper/ScreenPolygon.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/RasterRectangle2D.h"

typedef unsigned long RasterDepthValue;

/**
Largest depth of the scan conversion of PolygonScanConverter, the depth of an
empty sample.

C++ port note: in the cpp port this constant is
`PolygonScanConverter::MAXIMUM_DEPTH`; here PolygonScanConverter is a class
template (see below), and the compilers of this port have no static data
members in class templates, so the constant is in this class of its own,
defined in PolygonScanConverter.cpp.
*/
class PolygonScanConverterDepth {
  private:
    // No instances
    PolygonScanConverterDepth();

  public:
    static const RasterDepthValue MAXIMUM_DEPTH;
};

/**
Point-sampled scan conversion of convex polygons after [HECK1990], writing one
payload per covered sample, with or without a depth test. It keeps no state. The template
implementations are in PolygonScanFlat.txx and PolygonScanZ.txx.

C++ port note: legacy compilers (gcc 2.7, Turbo C++ 3.0) have no member
templates, so the class itself is a template on the Sample payload (as
`Collections<T>`), i.e. `PolygonScanConverter<Sample>::scanZ(...)`, where
the cpp port has a class with static member templates; its non-template
members (incrementalizeYFlat and incrementalizeYZ) are members of the class
template here too, and its MAXIMUM_DEPTH is
PolygonScanConverterDepth::MAXIMUM_DEPTH.
*/
template <class Sample>
class PolygonScanConverter {
  private:
    static void incrementalizeYFlat(const PolygonVertex &p1, const PolygonVertex &p2, int y, double &sx, double &dsx);
    static void scanlineFlat(Sample *samples, int stride, Sample value,
                             int y, double lSx, double rSx, const RasterRectangle2D *win);

    static RasterDepthValue depthValue(double depth);
    static void incrementalizeYZ(const PolygonVertex &p1, const PolygonVertex &p2, int y,
                                 double &sx, double &dsx, double &sz, double &dsz);
    static void scanlineZ(Sample *samples, RasterDepthValue *depth, int stride, Sample value,
                          int y, double lSx, double lSz, double rSx, double rSz, const RasterRectangle2D *win);

    // No instances (a deleted constructor in the cpp port)
    PolygonScanConverter();

  public:
    static void scanFlat(Sample *samples, int stride, Sample value, const ScreenPolygon *p, const RasterRectangle2D *win);

    static void scanZ(Sample *samples, RasterDepthValue *depth, int stride, Sample value,
                      const ScreenPolygon *p, const RasterRectangle2D *window);
};

#endif
