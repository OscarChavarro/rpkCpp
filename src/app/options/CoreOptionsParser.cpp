#include "app/options/CoreOptionsParser.h"

#include "common/RenderOptions.h"
#include "io/context/ParseSession.h"
#include "scene/Scene.h"
#include "tonemap/ToneMappingContext.h"
#include "app/options/CommandLine.h"

void
CoreOptionsParser::parse(
        int *argc,
        char **argv,
        ParseSession &parseSession,
        Scene &scene,
        RenderOptions &renderOptions,
        ToneMappingContext &toneMapOptions,
        int &imageOutputWidth,
        int &imageOutputHeight,
        bool &glutDebugEnabled,
        char *toneMapNameOut,
        OptionsType &optionTypes)
{
    CommandLine::commandLineGeneralProgramParseOptions(
        argc,
        argv,
        &parseSession.singleSided,
        &parseSession.numberOfQuarterCircleDivisions,
        &imageOutputWidth,
        &imageOutputHeight,
        &glutDebugEnabled,
        optionTypes);

    CommandLine::renderParseOptions(argc, argv, &renderOptions, optionTypes);
    renderOptions.toneMapOptions = &toneMapOptions;
    CommandLine::toneMapParseOptions(argc, argv, toneMapNameOut, toneMapOptions, optionTypes);
    CommandLine::cameraParseOptions(argc, argv, scene.camera, imageOutputWidth, imageOutputHeight, optionTypes);
}

Background *
CoreOptionsParser::createBackground() {
    return CommandLine::commandLineCreateBackground();
}
