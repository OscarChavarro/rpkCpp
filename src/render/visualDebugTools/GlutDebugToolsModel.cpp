#include "render/visualDebugTools/GlutDebugToolsModel.h"

GlutDebugToolsModel::GlutDebugToolsModel():
    mode(GlutDebugMode::RADIANCE_SCENE),
    width(1920),
    height(1200),
    scene(nullptr),
    radianceMethod(nullptr),
    renderOptions(nullptr),
    memoryFreeCallBack(nullptr),
    mgfContext(nullptr)
{
}
