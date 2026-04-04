#include "app/options/CoreOptionsParser.h"

#include "common/RenderOptions.h"
#include "io/context/ParseRuntimeContext.h"
#include "scene/Scene.h"
#include "tonemap/ToneMappingContext.h"
#include "app/options/CommandLine.h"

void
CoreOptionsParser::parse(
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
    CommandLine::commandLineGeneralProgramParseOptions(
        argc,
        argv,
        &parseSession.singleSided,
        &parseSession.numberOfQuarterCircleDivisions,
        &imageOutputWidth,
        &imageOutputHeight,
        &glutDebugEnabled);

    CommandLine::renderParseOptions(argc, argv, &renderOptions);
    CommandLine::toneMapParseOptions(argc, argv, toneMapNameOut, toneMapOptions);
    CommandLine::cameraParseOptions(argc, argv, scene.camera, imageOutputWidth, imageOutputHeight);
}

Background *
CoreOptionsParser::createBackground() {
    return CommandLine::commandLineCreateBackground();
}
