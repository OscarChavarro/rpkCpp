#ifndef COMMAND_LINE_CORE_OPTIONS_GROUP__
#define COMMAND_LINE_CORE_OPTIONS_GROUP__

#include "options/EnumBackgroundMode.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

class Background;

class OptionsGroupCore final {
  public:
    static void parse(
        int *argc,
        char **argv,
        ParseRuntimeContext &parseSession,
        Scene &scene,
        RendererConfiguration &renderOptions,
        ToneMappingContext &toneMapOptions,
        int &imageOutputWidth,
        int &imageOutputHeight,
        bool &glutDebugEnabled,
        char *toneMapNameOut);
    static Background *createBackground();

    static Background *commandLineCreateBackground();
    static void commandLineGeneralProgramParseOptions(
        int *argc,
        char **argv,
        bool *oneSidedSurfaces,
        int *conicSubDivisions,
        int *imageOutputWidth,
        int *imageOutputHeight,
        bool *glutDebugEnabledOut);

  private:
    static constexpr int DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS = 4;
    static constexpr bool DEFAULT_FORCE_ONE_SIDED = true;
    static const ColorRgb DEFAULT_BACKGROUND_COLOR;

    static int numberOfQuarterCircleDivisions;
    static int fileOptionsForceOneSidedSurfaces;
    static int outputImageWidth;
    static int outputImageHeight;
    static int glutDebugEnabled;
    static EnumBackgroundMode backgroundMode;
    static ColorRgb backgroundColor;

    static bool commandLineParseFloat(const char *text, float *value);
    static bool commandLineParseBackgroundColor(const char *rArg, const char *gArg, const char *bArg, ColorRgb *color);
    static void commandLineParseBackgroundOption(int *argc, char **argv);
    static char toLowerAscii(char c);
    static bool equalsIgnoreCase(const char *a, const char *b);
    static void mainForceOneSidedOption(int &value);
    static void mainMonochromeOption(int &value);
    static void setIntTrue(int &value);
};

#endif
