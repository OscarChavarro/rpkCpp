#ifndef _SOFT_IDS_WRAPPER__
#define _SOFT_IDS_WRAPPER__

#include "vsdk/toolkit/render/FaceVisibilityBuffer.h"
#include "vsdk/toolkit/scene/Scene.h"

class SoftIdsWrapper {
  private:
    const Scene &scene;
    FaceVisibilityBuffer target;

  public:
    explicit SoftIdsWrapper(const Scene *inScene, const RendererConfiguration *renderOptions);

    void getSize(long *width, long *height) const;
    const Patch *getPatchAtPixel(int x, int y) const;
};

inline void
SoftIdsWrapper::getSize(long *width, long *height) const {
    *width = target.getWidth();
    *height = target.getHeight();
}

inline const Patch *
SoftIdsWrapper::getPatchAtPixel(int x, int y) const {
    const SglFaceIndex face = target.getSample(x, target.getHeight() - 1 - y);
    return face.isBackground() ? nullptr : scene.getFace(face.getIndex());
}

#endif
