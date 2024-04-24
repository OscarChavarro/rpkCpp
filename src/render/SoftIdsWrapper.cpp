#include "java/util/ArrayList.txx"
#include "render/SoftIdsWrapper.h"

Soft_ID_Renderer::~Soft_ID_Renderer() {
    delete sgl;
}

void
Soft_ID_Renderer::init(Scene *scene) {
    SGL_CONTEXT *oldSglContext = GLOBAL_sgl_currentContext;
    sgl = setupSoftFrameBuffer(scene->camera);
    softRenderPatches(scene, &GLOBAL_render_renderOptions);
    sglMakeCurrent(oldSglContext); // Make the old one current again
}
