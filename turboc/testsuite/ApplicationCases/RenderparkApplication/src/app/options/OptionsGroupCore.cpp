#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "java/lang/System.h"
#include "scene/ConstantColorBackground.h"
#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/EnumAppOptions.h"
#include "app/options/OptionsGroupCore.h"
#include "app/options/OptionsGroupRender.h"
#include "app/options/OptionsGroupToneMapping.h"
#include "app/options/OptionsGroupCamera.h"

static char
optionsGroupCoreToLowerAscii(char c) {
    if ( c >= 'A' && c <= 'Z' ) {
        return ((char)(c - 'A' + 'a'));
    }
    return c;
}

static bool
optionsGroupCoreEqualsIgnoreCase(const char *a, const char *b) {
    if ( a == NULL || b == NULL ) {
        return false;
    }
    unsigned long i = 0;
    while ( a[i] != '\0' && b[i] != '\0' ) {
        if ( optionsGroupCoreToLowerAscii(a[i]) != optionsGroupCoreToLowerAscii(b[i]) ) {
            return false;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

const ColorRgb OptionsGroupCore::DEFAULT_BACKGROUND_COLOR(0.0, 0.0, 0.0);
int OptionsGroupCore::numberOfQuarterCircleDivisions = DF_NUM_O_QRTC_DVSNS;
int OptionsGroupCore::fileOptsFrcOneSddSrfcs = 0;
int OptionsGroupCore::outputImageWidth = 1920;
int OptionsGroupCore::outputImageHeight = 1080;
int OptionsGroupCore::glutDebugEnabled = false;
EnumBackgroundMode OptionsGroupCore::backgroundMode = NONE;
ColorRgb OptionsGroupCore::backgroundColor = OptionsGroupCore::DEFAULT_BACKGROUND_COLOR;

void
OptionsGroupCore::parse(
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
    OptionsGroupCore::cmmndLineGenProgParseOpts(
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
OptionsGroupCore::createBackground() {
    return OptionsGroupCore::commandLineCreateBackground();
}

Background *
OptionsGroupCore::commandLineCreateBackground() {
    if ( backgroundMode == SOLID ) {
        return new ConstantColorBackground(backgroundColor);
    }
    return NULL;
}

bool
OptionsGroupCore::commandLineParseFloat(const char *text, float *value) {
    if ( text == NULL || value == NULL ) {
        return false;
    }

    char *endPointer = NULL;
    const float parsedValue = strtof(text, &endPointer);
    if ( endPointer == text || *endPointer != '\0' ) {
        return false;
    }

    *value = parsedValue;
    return true;
}

bool
OptionsGroupCore::commandLineParseBackgroundColor(const char *rArg, const char *gArg, const char *bArg, ColorRgb *color) {
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    if ( !OptionsGroupCore::commandLineParseFloat(rArg, &red)
         || !OptionsGroupCore::commandLineParseFloat(gArg, &green)
         || !OptionsGroupCore::commandLineParseFloat(bArg, &blue) ) {
        return false;
    }

    if ( red < 0.0f || red > 1.0f || green < 0.0f || green > 1.0f || blue < 0.0f || blue > 1.0f ) {
        return false;
    }

    *color = ColorRgb(red, green, blue);
    return true;
}

void
OptionsGroupCore::cmmndLineParseBgOpt(int *argc, char **argv) {
    int writeIndex = 0;
    int readIndex = 0;
    while ( readIndex < *argc ) {
        const char *argument = argv[readIndex];
        if ( argument == NULL || strcmp(argument, "-background") != 0 ) {
            argv[writeIndex++] = argv[readIndex++];
            continue;
        }

        if ( readIndex + 1 >= *argc ) {
            System::err.printf("Option '-background' requires a mode. Supported mode: solid.\n");
            readIndex += 1;
            continue;
        }

        const char *mode = argv[readIndex + 1];
        if ( !optionsGroupCoreEqualsIgnoreCase(mode, "solid") ) {
            System::err.printf(
                "Invalid background mode '%s'. Expected '-background solid <r> <g> <b>'.\n",
                mode);
            readIndex += 2;
            continue;
        }

        if ( readIndex + 4 >= *argc ) {
            System::err.printf(
                "Option '-background solid' requires three values in range [0.0, 1.0].\n");
            readIndex += 2;
            continue;
        }

        ColorRgb parsedColor;
        if ( !OptionsGroupCore::commandLineParseBackgroundColor(
                 argv[readIndex + 2],
                 argv[readIndex + 3],
                 argv[readIndex + 4],
                 &parsedColor) ) {
            System::err.printf(
                "Invalid '-background solid' color. Use '-background solid <r> <g> <b>' with values in [0.0, 1.0].\n");
        } else {
            backgroundMode = SOLID;
            backgroundColor = parsedColor;
        }
        readIndex += 5;
    }

    while ( writeIndex < *argc ) {
        argv[writeIndex++] = NULL;
    }
    *argc = writeIndex;
}

void
OptionsGroupCore::mainForceOneSidedOption(int &value) {
    fileOptsFrcOneSddSrfcs = value;
}

void
OptionsGroupCore::mainMonochromeOption(int &value) {
    numberOfQuarterCircleDivisions = value;
}

void
OptionsGroupCore::setIntTrue(int &value) {
    value = 1;
}

void
OptionsGroupCore::cmmndLineGenProgParseOpts(
        int *argc,
        char **argv,
        bool *oneSidedSurfaces,
        int *conicSubDivisions,
        int *imageOutputWidth,
        int *imageOutputHeight,
        bool *glutDebugEnabledOut)
{
    EnumAppOptions appOptions;
    appOptions.width = outputImageWidth;
    appOptions.height = outputImageHeight;
    appOptions.nqcdivs = numberOfQuarterCircleDivisions;
    appOptions.yesValue = 1;
    appOptions.noValue = 0;
    appOptions.debug = 0;
    TypedOption<int> widthOpt("-width", &appOptions.width, 1, NULL, NULL);
    TypedOption<int> heightOpt("-height", &appOptions.height, 1, NULL, NULL);
    TypedOption<int> nqcdivsOpt("-nqcdivs", &appOptions.nqcdivs, 1, NULL, NULL);
    TypedOption<int> forceOneSidedOpt("-force-onesided", &appOptions.yesValue, 0, OptionsGroupCore::mainForceOneSidedOption, NULL);
    TypedOption<int> dontForceOneSidedOpt("-dont-force-onesided", &appOptions.noValue, 0, OptionsGroupCore::mainForceOneSidedOption, NULL);
    TypedOption<int> monochromaticOpt("-monochromatic", &appOptions.yesValue, 0, OptionsGroupCore::mainMonochromeOption, NULL);
    TypedOption<int> glutDebugOpt("-glutDebug", &appOptions.debug, 0, OptionsGroupCore::setIntTrue, NULL);
    OptionBase registry[] = {
        REGISTER_OPTION(int, widthOpt, 5),
        REGISTER_OPTION(int, heightOpt, 6),
        REGISTER_OPTION(int, nqcdivsOpt, 3),
        REGISTER_OPTION(int, forceOneSidedOpt, 10),
        REGISTER_OPTION(int, dontForceOneSidedOpt, 14),
        REGISTER_OPTION(int, monochromaticOpt, 5),
        REGISTER_OPTION(int, glutDebugOpt, 6)
    };

    fileOptsFrcOneSddSrfcs = DEFAULT_FORCE_ONE_SIDED;
    numberOfQuarterCircleDivisions = DF_NUM_O_QRTC_DVSNS;
    backgroundMode = NONE;
    backgroundColor = DEFAULT_BACKGROUND_COLOR;
    glutDebugEnabled = appOptions.debug;
    OptionsGroupCore::cmmndLineParseBgOpt(argc, argv);
    OptionGroup generalGroups[] = {
        OptionGroup("global", registry, 7)
    };
    OptionParser<OptionBase>::parse(argc, argv, generalGroups, 1, &appOptions);

    outputImageWidth = appOptions.width;
    outputImageHeight = appOptions.height;
    numberOfQuarterCircleDivisions = appOptions.nqcdivs;
    glutDebugEnabled = appOptions.debug;

    if ( fileOptsFrcOneSddSrfcs != 0 ) {
        *oneSidedSurfaces = true;
    } else {
        *oneSidedSurfaces = false;
    }
    *conicSubDivisions = numberOfQuarterCircleDivisions;
    *imageOutputWidth = outputImageWidth;
    *imageOutputHeight = outputImageHeight;
    *glutDebugEnabledOut = glutDebugEnabled;

#ifndef OPEN_GL_ENABLED
    if ( glutDebugEnabled ) {
        System::err.printf(
            "ERROR: Option '-glutDebug' requires OpenGL support. Recompile with -DOPEN_GL_ENABLED=ON.\n");
        System::err.flush();
        System::exit(1);
    }
#endif
}
