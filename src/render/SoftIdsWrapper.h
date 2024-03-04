#ifndef _CXX_SOFTIDS_H_
#define _CXX_SOFTIDS_H_

#include "java/util/ArrayList.h"
#include "render/softids.h"

class Soft_ID_Renderer {
  protected:
    SGL_CONTEXT *sgl; // Software rendering context, includes frame buffer

    void init(java::ArrayList<Patch *> *scenePatches); // Also performs the actual ID rendering
  public:
    explicit Soft_ID_Renderer(java::ArrayList<Patch *> *scenePatches) {
        sgl = nullptr;
        init(scenePatches);
    }

    ~Soft_ID_Renderer();

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
