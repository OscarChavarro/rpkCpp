#ifndef _SOFT_IDS_WRAPPER__
#define _SOFT_IDS_WRAPPER__

#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/render/Softids.h"

class SoftIdsWrapper {
  private:
    SglContext *sgl; // Software rendering context, includes frame buffer

    void init(const Scene *scene, const RendererConfiguration *renderOptions); // Also performs the actual ID rendering

  public:
    explicit SoftIdsWrapper(const Scene *scene, const RendererConfiguration *renderOptions);
    ~SoftIdsWrapper();

    void getSize(long *width, long *height) const;
    Patch *getPatchAtPixel(int x, int y) const;
};

inline void
SoftIdsWrapper::getSize(long *width, long *height) const {
    *width = sgl->width;
    *height = sgl->height;
}

inline Patch *
SoftIdsWrapper::getPatchAtPixel(int x, int y) const {
    int index = (sgl->height - 1 - y) * sgl->width + x;
    return sgl->patchBuffer[index];
}

#endif
