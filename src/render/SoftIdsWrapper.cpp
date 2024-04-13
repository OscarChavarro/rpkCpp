#include "java/util/ArrayList.txx"
#include "render/SoftIdsWrapper.h"

Soft_ID_Renderer::~Soft_ID_Renderer() {
    delete sgl;
}

void
Soft_ID_Renderer::init(
    java::ArrayList<Patch *> *scenePatches,
    Geometry *clusteredWorldGeometry)
{
    SGL_CONTEXT *oldSglContext = GLOBAL_sgl_currentContext;
    sgl = setupSoftFrameBuffer();
    softRenderPatches(scenePatches, clusteredWorldGeometry);
    sglMakeCurrent(oldSglContext); // Make the old one current again
}

