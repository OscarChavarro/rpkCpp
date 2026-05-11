#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/scene/ConstantColorBackground.h"
#include "vsdk/toolkit/common/commandLineOptions/OptionParser.h"
#include "vsdk/toolkit/common/commandLineOptions/TypedOption.h"
#include "options/EnumAppOptions.h"
#include "options/OptionsGroupCore.h"
#include "options/OptionsGroupRender.h"
#include "options/OptionsGroupToneMapping.h"
#include "options/OptionsGroupCamera.h"

char
OptionsGroupCore::toLowerAscii(char c) {
    if ( c >= 'A' && c <= 'Z' ) {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

bool
OptionsGroupCore::equalsIgnoreCase(const char *a, const char *b) {
    if ( a == nullptr || b == nullptr ) {
        return false;
    }
    unsigned long i = 0;
    while ( a[i] != '\0' && b[i] != '\0' ) {
        if ( OptionsGroupCore::toLowerAscii(a[i]) != OptionsGroupCore::toLowerAscii(b[i]) ) {
            return false;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

const ColorRgb OptionsGroupCore::DEFAULT_BACKGROUND_COLOR(0.0, 0.0, 0.0);
int OptionsGroupCore::numberOfQuarterCircleDivisions = OptionsGroupCore::DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
int OptionsGroupCore::fileOptionsForceOneSidedSurfaces = 0;
int OptionsGroupCore::outputImageWidth = 1920;
int OptionsGroupCore::outputImageHeight = 1080;
int OptionsGroupCore::glutDebugEnabled = false;
EnumBackgroundMode OptionsGroupCore::backgroundMode = EnumBackgroundMode::NONE;
ColorRgb OptionsGroupCore::backgroundColor = OptionsGroupCore::DEFAULT_BACKGROUND_COLOR;

void
OptionsGroupCore::parse(
    int *argc,
    char **argv,
    ParseRuntimeContext &parseSession,
    Scene &scene,
    RendererConfiguration &renderOptions,
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
OptionsGroupCore::createBackground() {
    return OptionsGroupCore::commandLineCreateBackground();
}

Background *
OptionsGroupCore::commandLineCreateBackground() {
    if ( backgroundMode == EnumBackgroundMode::SOLID ) {
        return new ConstantColorBackground(backgroundColor);
    }
    return nullptr;
}

bool
OptionsGroupCore::commandLineParseFloat(const char *text, float *value) {
    if ( text == nullptr || value == nullptr ) {
        return false;
    }

    char *endPointer = nullptr;
    const float parsedValue = strtof(text, &endPointer);
    if ( endPointer == text || *endPointer != '\0' ) {
        return false;
    }

    *value = parsedValue;
    return true;
}

bool
OptionsGroupCore::commandLineParseBackgroundColor(const char *rArg, const char *gArg, const char *bArg, ColorRgb *color) {
    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;
    if ( !OptionsGroupCore::commandLineParseFloat(rArg, &red)
         || !OptionsGroupCore::commandLineParseFloat(gArg, &green)
         || !OptionsGroupCore::commandLineParseFloat(bArg, &blue) ) {
        return false;
    }

    if ( red < 0.0F || red > 1.0F || green < 0.0F || green > 1.0F || blue < 0.0F || blue > 1.0F ) {
        return false;
    }

    color->set(red, green, blue);
    return true;
}

void
OptionsGroupCore::commandLineParseBackgroundOption(int *argc, char **argv) {
    int writeIndex = 0;
    int readIndex = 0;
    while ( readIndex < *argc ) {
        const char *argument = argv[readIndex];
        if ( argument == nullptr || strcmp(argument, "-background") != 0 ) {
            argv[writeIndex++] = argv[readIndex++];
            continue;
        }

        if ( readIndex + 1 >= *argc ) {
            java::System::err.printf("Option '-background' requires a mode. Supported mode: solid.\n");
            readIndex += 1;
            continue;
        }

        const char * const mode = argv[readIndex + 1];
        if ( !OptionsGroupCore::equalsIgnoreCase(mode, "solid") ) {
            java::System::err.printf(
                "Invalid background mode '%s'. Expected '-background solid <r> <g> <b>'.\n",
                mode);
            readIndex += 2;
            continue;
        }

        if ( readIndex + 4 >= *argc ) {
            java::System::err.printf(
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
            java::System::err.printf(
                "Invalid '-background solid' color. Use '-background solid <r> <g> <b>' with values in [0.0, 1.0].\n");
        } else {
            backgroundMode = EnumBackgroundMode::SOLID;
            backgroundColor = parsedColor;
        }
        readIndex += 5;
    }

    while ( writeIndex < *argc ) {
        argv[writeIndex++] = nullptr;
    }
    *argc = writeIndex;
}

void
OptionsGroupCore::mainForceOneSidedOption(int &value) {
    fileOptionsForceOneSidedSurfaces = value;
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
OptionsGroupCore::commandLineGeneralProgramParseOptions(
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

    TypedOption<int> widthOpt = {"-width", &appOptions.width, 1, nullptr, nullptr};
    TypedOption<int> heightOpt = {"-height", &appOptions.height, 1, nullptr, nullptr};
    TypedOption<int> nqcdivsOpt = {"-nqcdivs", &appOptions.nqcdivs, 1, nullptr, nullptr};
    TypedOption<int> forceOneSidedOpt = {"-force-onesided", &appOptions.yesValue, 0, OptionsGroupCore::mainForceOneSidedOption, nullptr};
    TypedOption<int> dontForceOneSidedOpt = {"-dont-force-onesided", &appOptions.noValue, 0, OptionsGroupCore::mainForceOneSidedOption, nullptr};
    TypedOption<int> monochromaticOpt = {"-monochromatic", &appOptions.yesValue, 0, OptionsGroupCore::mainMonochromeOption, nullptr};
    TypedOption<int> glutDebugOpt = {"-glutDebug", &appOptions.debug, 0, OptionsGroupCore::setIntTrue, nullptr};
    OptionBase registry[] = {
        REGISTER_OPTION(int, widthOpt, 5),
        REGISTER_OPTION(int, heightOpt, 6),
        REGISTER_OPTION(int, nqcdivsOpt, 3),
        REGISTER_OPTION(int, forceOneSidedOpt, 10),
        REGISTER_OPTION(int, dontForceOneSidedOpt, 14),
        REGISTER_OPTION(int, monochromaticOpt, 5),
        REGISTER_OPTION(int, glutDebugOpt, 6)
    };

    fileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED;
    numberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    backgroundMode = EnumBackgroundMode::NONE;
    backgroundColor = DEFAULT_BACKGROUND_COLOR;
    glutDebugEnabled = appOptions.debug;
    OptionsGroupCore::commandLineParseBackgroundOption(argc, argv);
    OptionGroup generalGroups[] = {
        OptionGroup("global", registry, 7)
    };
    OptionParser<OptionBase>::parse(argc, argv, generalGroups, 1, &appOptions);

    outputImageWidth = appOptions.width;
    outputImageHeight = appOptions.height;
    numberOfQuarterCircleDivisions = appOptions.nqcdivs;
    glutDebugEnabled = appOptions.debug;

    if ( fileOptionsForceOneSidedSurfaces != 0 ) {
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
        java::System::err.printf(
            "ERROR: Option '-glutDebug' requires OpenGL support. Rebuild with CMake flag '-DWITH_OPENGL=ON'.\n");
        java::System::err.flush();
        java::System::exit(1);
    }
#endif
}
