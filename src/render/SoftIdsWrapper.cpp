#include "java/util/ArrayList.txx"
#include "render/SoftIdsWrapper.h"

Soft_ID_Renderer::~Soft_ID_Renderer() {
    sglClose(sgl);
}

void
Soft_ID_Renderer::init(java::ArrayList<Patch *> *scenePatches) {
    SGL_CONTEXT *oldSglContext = GLOBAL_sgl_currentContext;
    sgl = setupSoftFrameBuffer();
    softRenderPatches(scenePatches);
    sglMakeCurrent(oldSglContext); // Make the old one current again
}

