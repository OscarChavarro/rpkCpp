#include "java/util/ArrayList.txx"
#include "render/SoftIdsWrapper.h"

SoftIdsWrapper::SoftIdsWrapper(const Scene *scene, const RenderOptions *renderOptions) {
    sgl = nullptr;
    init(scene, renderOptions);
}

SoftIdsWrapper::~SoftIdsWrapper() {
    delete sgl;
}

void
SoftIdsWrapper::init(const Scene *scene, const RenderOptions *renderOptions) {
    sgl = setupSoftFrameBuffer(scene->camera);
    softRenderPatches(scene, renderOptions, sgl);
}
