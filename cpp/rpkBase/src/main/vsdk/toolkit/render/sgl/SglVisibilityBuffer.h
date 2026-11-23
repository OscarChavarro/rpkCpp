#ifndef SGL_VISIBILITY_BUFFER__
#define SGL_VISIBILITY_BUFFER__

#include "vsdk/toolkit/render/sgl/SglContext.h"

/** A single payload per sample. Depth, when enabled, belongs to this pass. */
template<class Sample>
class SglVisibilityBuffer {
  private:
    SglContext context;
    const Sample background;
    Sample *const samples;

  public:
    SglVisibilityBuffer(int width, int height, Sample backgroundValue, bool depthTesting = true);
    ~SglVisibilityBuffer();
    SglVisibilityBuffer(const SglVisibilityBuffer &) = delete;
    SglVisibilityBuffer &operator=(const SglVisibilityBuffer &) = delete;

    int getWidth() const { return context.getWidth(); }
    int getHeight() const { return context.getHeight(); }
    const Window &getViewport() const { return context.getViewport(); }
    int getViewportWidth() const { return context.getViewportWidth(); }
    int getViewportHeight() const { return context.getViewportHeight(); }
    Sample getSample(int x, int y) const { return samples[y * getWidth() + x]; }

    void setViewport(int x, int y, int width, int height) { context.setViewport(x, y, width, height); }
    void setTransform(const Matrix4x4f &matrix) { context.setTransform(matrix); }
    void clear();
    void drawPolygon(int numberOfVertices, const Vector3Df *vertices, Sample value);
};

#endif
