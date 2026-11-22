#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/render/SoftIdsWrapper.h"

SoftIdsWrapper::SoftIdsWrapper(const Scene *scene, const RendererConfiguration *renderOptions) {
    sgl = nullptr;
    init(scene, renderOptions);
}

SoftIdsWrapper::~SoftIdsWrapper() {
    delete sgl;
}

void
SoftIdsWrapper::init(const Scene *scene, const RendererConfiguration *renderOptions) {
    sgl = SoftIds::setupSoftFrameBuffer(scene->camera);
    SoftIds::softRenderPatches(scene, renderOptions, sgl);
}
