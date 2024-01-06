// SoftIds.H: C++ wrapper for softids.h

#ifndef _CXX_SOFTIDS_H_
#define _CXX_SOFTIDS_H_

#include "shared/softids.h"

class Soft_ID_Renderer {
protected:
    SGL_CONTEXT *sgl; // the software rendering context,
                      // includes frame buffer

    void init(); // also performs the actual ID rendering.

public:
    Soft_ID_Renderer() {
        sgl = 0;
        init();
    }

    ~Soft_ID_Renderer();

    inline void
    get_size(long *width, long *height) {
        *width = sgl->width;
        *height = sgl->height;
    }

    inline PATCH *
    get_patch_at_pixel(int x, int y) {
        return (PATCH *) (sgl->frameBuffer[(sgl->height - 1 - y) * sgl->width + x]);
    }

    inline PATCH **
    get_patches_at_scanline(int y) {
        return (PATCH **) &(sgl->frameBuffer[(sgl->height - 1 - y) * sgl->width]);
    }
};

#endif
