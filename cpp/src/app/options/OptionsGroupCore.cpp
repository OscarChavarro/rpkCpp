#include <cstdlib>
#include <strings.h>

#include "java/lang/System.h"
#include "scene/ConstantColorBackground.h"
#include "common/commandLineOptions/OptionParser.h"
#include "app/options/EnumAppOptions.h"
#include "app/options/OptionsGroupCore.h"
#include "app/options/GeneralProgramOptions.h"

const ColorRgb OptionsGroupCore::DEFAULT_BACKGROUND_COLOR(0.0, 0.0, 0.0);
int OptionsGroupCore::numberOfQuarterCircleDivisions = OptionsGroupCore::DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
int OptionsGroupCore::fileOptionsForceOneSidedSurfaces = 0;
int OptionsGroupCore::outputImageWidth = 1920;
int OptionsGroupCore::outputImageHeight = 1080;
int OptionsGroupCore::glutDebugEnabled = false;
EnumBackgroundMode OptionsGroupCore::backgroundMode = EnumBackgroundMode::NONE;
ColorRgb OptionsGroupCore::backgroundColor = OptionsGroupCore::DEFAULT_BACKGROUND_COLOR;

ColorRgb
OptionsGroupCore::commandLineDefaultBackgroundColor() {
    return DEFAULT_BACKGROUND_COLOR;
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

        const char *mode = argv[readIndex + 1];
        if ( strcasecmp(mode, "solid") != 0 ) {
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
OptionsGroupCore::commandLineImageWidthOption(int &value) {
    outputImageWidth = value;
}

void
OptionsGroupCore::commandLineImageHeightOption(int &value) {
    outputImageHeight = value;
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
    GeneralProgramOptions optionsRegistry(
            OptionsGroupCore::mainForceOneSidedOption,
            OptionsGroupCore::mainMonochromeOption,
            OptionsGroupCore::setIntTrue);

    EnumAppOptions appOptions;
    appOptions.width = outputImageWidth;
    appOptions.height = outputImageHeight;
    appOptions.nqcdivs = numberOfQuarterCircleDivisions;
    appOptions.yesValue = 1;
    appOptions.noValue = 0;
    appOptions.debug = 0;

    fileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED;
    numberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    backgroundMode = EnumBackgroundMode::NONE;
    backgroundColor = DEFAULT_BACKGROUND_COLOR;
    glutDebugEnabled = appOptions.debug;
    OptionsGroupCore::commandLineParseBackgroundOption(argc, argv);
    OptionGroup generalGroups[] = {
        OptionGroup("global", optionsRegistry.entries(), optionsRegistry.count())
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
            "ERROR: Option '-glutDebug' requires OpenGL support. Recompile with -DOPEN_GL_ENABLED=ON.\n");
        java::System::err.flush();
        java::System::exit(1);
    }
#endif
}
