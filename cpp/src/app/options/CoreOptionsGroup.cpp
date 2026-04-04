#include "app/options/CoreOptionsGroup.h"

#include "common/RenderOptions.h"
#include "io/context/ParseRuntimeContext.h"
#include "scene/Scene.h"
#include "tonemap/ToneMappingContext.h"
#include "app/options/OptionsGroupCore.h"
#include "app/options/OptionsGroupRender.h"
#include "app/options/OptionsGroupToneMapping.h"
#include "app/options/OptionsGroupCamera.h"

void
CoreOptionsGroup::parse(
    int *argc,
    char **argv,
    ParseRuntimeContext &parseSession,
    Scene &scene,
    RenderOptions &renderOptions,
    ToneMappingContext &toneMapOptions,
    int &imageOutputWidth,
    int &imageOutputHeight,
    bool &glutDebugEnabled,
    char *toneMapNameOut)
{
    OptionsGroupCore::commandLineGeneralProgramParseOptions(
        argc,
        argv,
        &parseSession.singleSided,
        &parseSession.numberOfQuarterCircleDivisions,
        &imageOutputWidth,
        &imageOutputHeight,
        &glutDebugEnabled);

    OptionsGroupRender::renderParseOptions(argc, argv, &renderOptions);
    OptionsGroupToneMapping::toneMapParseOptions(argc, argv, toneMapNameOut, toneMapOptions);
    OptionsGroupCamera::cameraParseOptions(argc, argv, scene.camera, imageOutputWidth, imageOutputHeight);
}

Background *
CoreOptionsGroup::createBackground() {
    return OptionsGroupCore::commandLineCreateBackground();
}
