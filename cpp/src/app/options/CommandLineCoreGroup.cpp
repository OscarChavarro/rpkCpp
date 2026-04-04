#include <cstdlib>
#include <cstring>
#include <strings.h>

#include "java/lang/System.h"
#include "scene/ConstantColorBackground.h"
#include "common/commandLineOptions/OptionParser.h"
#include "app/options/AppOptions.h"
#include "app/options/CommandLine.h"
#include "app/options/GeneralProgramOptions.h"

namespace {

void setIntTrue(int &value) {
    value = 1;
}

}

ColorRgb
CommandLine::commandLineDefaultBackgroundColor() {
    return DEFAULT_BACKGROUND_COLOR;
}

Background *
CommandLine::commandLineCreateBackground() {
    if ( backgroundMode == BackgroundMode::SOLID ) {
        return new ConstantColorBackground(backgroundColor);
    }
    return nullptr;
}

bool
CommandLine::commandLineParseFloat(const char *text, float *value) {
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
CommandLine::commandLineParseBackgroundColor(const char *rArg, const char *gArg, const char *bArg, ColorRgb *color) {
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    if ( !CommandLine::commandLineParseFloat(rArg, &red)
         || !CommandLine::commandLineParseFloat(gArg, &green)
         || !CommandLine::commandLineParseFloat(bArg, &blue) ) {
        return false;
    }

    if ( red < 0.0f || red > 1.0f || green < 0.0f || green > 1.0f || blue < 0.0f || blue > 1.0f ) {
        return false;
    }

    color->set(red, green, blue);
    return true;
}

void
CommandLine::commandLineParseBackgroundOption(int *argc, char **argv) {
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
        if ( !CommandLine::commandLineParseBackgroundColor(
                 argv[readIndex + 2],
                 argv[readIndex + 3],
                 argv[readIndex + 4],
                 &parsedColor) ) {
            java::System::err.printf(
                "Invalid '-background solid' color. Use '-background solid <r> <g> <b>' with values in [0.0, 1.0].\n");
        } else {
            backgroundMode = BackgroundMode::SOLID;
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
CommandLine::mainForceOneSidedOption(int &value) {
    fileOptionsForceOneSidedSurfaces = value;
}

void
CommandLine::mainMonochromeOption(int &value) {
    numberOfQuarterCircleDivisions = value;
}

void
CommandLine::commandLineImageWidthOption(int &value) {
    outputImageWidth = value;
}

void
CommandLine::commandLineImageHeightOption(int &value) {
    outputImageHeight = value;
}

void
CommandLine::commandLineGeneralProgramParseOptions(
        int *argc,
        char **argv,
        bool *oneSidedSurfaces,
        int *conicSubDivisions,
        int *imageOutputWidth,
        int *imageOutputHeight,
        bool *glutDebugEnabledOut)
{
    GeneralProgramOptionsRegistry optionsRegistry(
        CommandLine::mainForceOneSidedOption,
        CommandLine::mainMonochromeOption,
        setIntTrue);

    AppOptions appOptions;
    appOptions.width = outputImageWidth;
    appOptions.height = outputImageHeight;
    appOptions.nqcdivs = numberOfQuarterCircleDivisions;
    appOptions.yesValue = 1;
    appOptions.noValue = 0;
    appOptions.debug = 0;

    fileOptionsForceOneSidedSurfaces = DEFAULT_FORCE_ONE_SIDED;
    numberOfQuarterCircleDivisions = DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    backgroundMode = BackgroundMode::NONE;
    backgroundColor = DEFAULT_BACKGROUND_COLOR;
    CommandLine::glutDebugEnabled = appOptions.debug;
    CommandLine::commandLineParseBackgroundOption(argc, argv);
    OptionGroup generalGroups[] = {
        OptionGroup("global", optionsRegistry.entries(), optionsRegistry.count())
    };
    OptionParser<OptionBase>::parse(argc, argv, generalGroups, 1, &appOptions);

    outputImageWidth = appOptions.width;
    outputImageHeight = appOptions.height;
    numberOfQuarterCircleDivisions = appOptions.nqcdivs;
    CommandLine::glutDebugEnabled = appOptions.debug;

    if ( fileOptionsForceOneSidedSurfaces != 0 ) {
        *oneSidedSurfaces = true;
    } else {
        *oneSidedSurfaces = false;
    }
    *conicSubDivisions = numberOfQuarterCircleDivisions;
    *imageOutputWidth = outputImageWidth;
    *imageOutputHeight = outputImageHeight;
    *glutDebugEnabledOut = CommandLine::glutDebugEnabled;

#ifndef OPEN_GL_ENABLED
    if ( CommandLine::glutDebugEnabled ) {
        java::System::err.printf(
            "ERROR: Option '-glutDebug' requires OpenGL support. Recompile with -DOPEN_GL_ENABLED=ON.\n");
        java::System::err.flush();
        java::System::exit(1);
    }
#endif
}
