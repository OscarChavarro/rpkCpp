// C++ wrapper for soft id rendering utilities in softids.c

#include "shared/SoftIdsWrapper.h"

static SGL_PIXEL
patchPointer(Patch *P) {
    return (SGL_PIXEL) P;
}

Soft_ID_Renderer::~Soft_ID_Renderer() {
    sglClose(sgl);
}

void
Soft_ID_Renderer::init() {
    SGL_CONTEXT *oldsgl = GLOBAL_sgl_currentContext;
    sgl = setupSoftFrameBuffer();
    softRenderPatches(patchPointer);
    sglMakeCurrent(oldsgl); // make the old one current again
}

