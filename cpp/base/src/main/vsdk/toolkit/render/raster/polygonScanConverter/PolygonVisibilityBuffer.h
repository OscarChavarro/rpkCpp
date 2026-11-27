#ifndef __POLYGON_VISIBILITY_BUFFER__
#define __POLYGON_VISIBILITY_BUFFER__

#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonRasterContext.h"

/** A single payload per sample. Depth, when enabled, belongs to this pass. */
template<class Sample>
class PolygonVisibilityBuffer {
  private:
    PolygonRasterContext context;
    const Sample background;
    Sample *const samples;

  public:
    PolygonVisibilityBuffer(int width, int height, Sample backgroundValue, bool depthTesting = true);
    ~PolygonVisibilityBuffer();
    PolygonVisibilityBuffer(const PolygonVisibilityBuffer &) = delete;
    PolygonVisibilityBuffer &operator=(const PolygonVisibilityBuffer &) = delete;

    int getWidth() const { return context.getWidth(); }
    int getHeight() const { return context.getHeight(); }
    const RasterRectangle2D &getViewport() const { return context.getViewport(); }
    int getViewportWidth() const { return context.getViewportWidth(); }
    int getViewportHeight() const { return context.getViewportHeight(); }
    Sample getSample(int x, int y) const { return samples[y * getWidth() + x]; }

    void setViewport(int x, int y, int width, int height) { context.setViewport(x, y, width, height); }
    void setTransform(const Matrix4x4d &matrix) { context.setTransform(matrix); }
    void clear();
    void drawPolygon(int numberOfVertices, const Vector3Dd *vertices, Sample value);
};

#endif
