#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __OPTIONS_GROUP_CORE__
#define __OPTIONS_GROUP_CORE__

#include "app/options/EnumBackgroundMode.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

class Background;

class OptionsGroupCore{ public:
    static void parse( int *argc, char **argv, ParseRuntimeContext &parseSession, Scene &scene, RenderOptions &renderOptions, ToneMappingContext &toneMapOptions, int &imageOutputWidth, int &imageOutputHeight, bool &glutDebugEnabled, char *toneMapNameOut);
    static Background *createBackground();

    static Background *commandLineCreateBackground();
    static void cmmndLineGenProgParseOpts( int *argc, char **argv, bool *oneSidedSurfaces, int *conicSubDivisions, int *imageOutputWidth, int *imageOutputHeight, bool *glutDebugEnabledOut);

  private:
    #define DF_NUM_O_QRTC_DVSNS 4
    #define DEFAULT_FORCE_ONE_SIDED true
    static const ColorRgb DEFAULT_BACKGROUND_COLOR;

    static int numberOfQuarterCircleDivisions;
    static int fileOptsFrcOneSddSrfcs;
    static int outputImageWidth;
    static int outputImageHeight;
    static int glutDebugEnabled;
    static EnumBackgroundMode backgroundMode;
    static ColorRgb backgroundColor;
    static SceneAccelerationMethod sceneAccelerationMethod;

    static bool commandLineParseFloat(const char *text, double *value);
    static bool commandLineParseBackgroundColor(const char *rArg, const char *gArg, const char *bArg, ColorRgb *color);
    static void cmmndLineParseBgOpt(int *argc, char **argv);
    static void mainForceOneSidedOption(int &value);
    static void mainMonochromeOption(int &value);
    static void setIntTrue(int &value);
    static void sceneAccelerationOption(char *&name);
};

#endif
