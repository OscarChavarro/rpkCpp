#ifndef SGL_CONTEXT__
#define SGL_CONTEXT__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4f.h"
#include "vsdk/toolkit/render/sgl/SglConstants.h"
#include "vsdk/toolkit/render/sgl/Window.h"

class Polygon;

/** Projection, viewport and optional depth plane shared by typed SGL targets. */
class SglContext {
  private:
    const int width;
    const int height;
    Window viewport;
    Matrix4x4f transform;
    SglDepthValue *const depthBuffer;

  public:
    SglContext(int inWidth, int inHeight, bool depthTesting);
    ~SglContext();
    SglContext(const SglContext &) = delete;
    SglContext &operator=(const SglContext &) = delete;

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getViewportWidth() const { return viewport.x1 - viewport.x0 + 1; }
    int getViewportHeight() const { return viewport.y1 - viewport.y0 + 1; }
    const Window &getViewport() const { return viewport; }
    SglDepthValue *getDepthBuffer() { return depthBuffer; }

    void setViewport(int x, int y, int viewportWidth, int viewportHeight);
    void setTransform(const Matrix4x4f &matrix) { transform = matrix; }
    bool projectPolygon(int numberOfVertices, const Vector3Df *vertices, Polygon &result) const;
};

#endif
