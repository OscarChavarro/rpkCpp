#include "java/util/ArrayList.txx"
#include "shared/options.h"
#include "shared/SoftIdsWrapper.h"

static SGL_PIXEL
patchPointer(Patch *P) {
    return (SGL_PIXEL) P;
}

Soft_ID_Renderer::~Soft_ID_Renderer() {
    sglClose(sgl);
}

void
Soft_ID_Renderer::init(java::ArrayList<Patch *> *scenePatches) {
    SGL_CONTEXT *oldSglContext = GLOBAL_sgl_currentContext;
    sgl = setupSoftFrameBuffer();
    softRenderPatches(patchPointer, scenePatches);
    sglMakeCurrent(oldSglContext); // Make the old one current again
}

