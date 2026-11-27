#include "vsdk/toolkit/render/SoftIdsWrapper.h"
#include "vsdk/toolkit/render/Softids.h"

SoftIdsWrapper::SoftIdsWrapper(const Scene *inScene, const RenderOptions *renderOptions):
    scene(*inScene),
    target(scene.camera->xSize, scene.camera->ySize, FaceIndexSample())
{
    SoftIds::renderVisibleFaces(scene, renderOptions, target);
}
