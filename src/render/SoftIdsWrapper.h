#ifndef _SOFT_IDS_WRAPPER__
#define _SOFT_IDS_WRAPPER__

#include "java/util/ArrayList.h"
#include "render/softids.h"

class SoftIdsWrapper {
  protected:
    SGL_CONTEXT *sgl; // Software rendering context, includes frame buffer

    void init(const Scene *scene, const RenderOptions *renderOptions); // Also performs the actual ID rendering

  public:
    explicit SoftIdsWrapper(const Scene *scene, const RenderOptions *renderOptions);
    ~SoftIdsWrapper();

    inline void
    getSize(long *width, long *height) {
        *width = sgl->width;
        *height = sgl->height;
    }

    inline Patch *
    getPatchAtPixel(int x, int y) {
        int index = (sgl->height - 1 - y) * sgl->width + x;
        return sgl->patchBuffer[index];
    }
};

#endif
