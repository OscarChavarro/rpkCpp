#ifndef __POLYGON_RASTER_CONTEXT__
#define __POLYGON_RASTER_CONTEXT__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonScanConverter.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/RasterRectangle2D.h"

/** Projection, viewport and optional depth plane shared by typed raster targets. */
class PolygonRasterContext {
  private:
    const int width;
    const int height;
    RasterRectangle2D viewport;
    Matrix4x4d transform;
    RasterDepthValue *const depthBuffer;

    // Not copyable (deleted in the cpp port)
    PolygonRasterContext(const PolygonRasterContext &);
    PolygonRasterContext &operator=(const PolygonRasterContext &);

  public:
    PolygonRasterContext(int inWidth, int inHeight, BOOLEAN depthTesting);
    ~PolygonRasterContext();

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getViewportWidth() const { return viewport.getMaxX() - viewport.getMinX() + 1; }
    int getViewportHeight() const { return viewport.getMaxY() - viewport.getMinY() + 1; }
    const RasterRectangle2D &getViewport() const { return viewport; }
    RasterDepthValue *getDepthBuffer() { return depthBuffer; }

    void setViewport(int x, int y, int viewportWidth, int viewportHeight);
    void setTransform(const Matrix4x4d &matrix) { transform = matrix; }
    ScreenPolygon projectPolygon(int numberOfVertices, const Vector3Dd *vertices) const;
};

#endif
