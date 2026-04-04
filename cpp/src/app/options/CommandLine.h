#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

#include "raycasting/common/RayTracer.h"
#include "common/RenderOptions.h"
#include "app/options/BatchOptions.h"
#include "app/options/BackgroundMode.h"
#include "app/options/EnumDesc.h"
#include "app/options/CommandLineOptions.h"
#include "raycasting/photonMap/PhotonMapState.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "tonemap/ToneMappingContext.h"

class CommandLine final {
  public:
    static ColorRgb commandLineDefaultBackgroundColor();
    static Background *commandLineCreateBackground();

    static void cameraParseOptions(
            int *argc,
            char **argv,
            Camera *camera,
            int imageWidth,
            int imageHeight);
    static void commandLineGeneralProgramParseOptions(
            int *argc,
            char **argv,
            bool *oneSidedSurfaces,
            int *conicSubDivisions,
            int *imageOutputWidth,
            int *imageOutputHeight,
            bool *glutDebugEnabledOut);
    static void stochasticRelaxationRadiosityParseOptions(
            int *argc,
            char **argv,
            StochasticRelaxation &stochasticRelaxationState,
            ElementHierarchyState &elementHierarchyState);
    static void randomWalkRadiosityParseOptions(
            int *argc,
            char **argv,
            StochasticRelaxation &stochasticRelaxationState);
    static void rayMattingParseOptions(
            int *argc,
            char **argv,
            RayMatterState &rayMatterState);
    static void stochasticRayTracerParseOptions(
            int *argc,
            char **argv,
            StochasticRayTracingState &stochasticRayTracingState);
    static void biDirectionalPathParseOptions(
            int *argc,
            char **argv,
            BidirectionalPathTracingState &bidirectionalPathState);
    static void photonMapParseOptions(
            int *argc,
            char **argv,
            PhotonMapState &photonMapState);
    static void toneMapParseOptions(
            int *argc,
            char **argv,
            char *toneMapNameOut,
            ToneMappingContext &toneMapOptionsContext);
    static void radianceMethodParseOptions(
            int *argc,
            char **argv,
            char *radianceMethodsStringOut);
    static void renderParseOptions(
            int *argc,
            char **argv,
            RenderOptions *renderOptions);
    static void batchParseOptions(
            int *argc,
            char **argv,
            BatchOptions *batchOptions);
    static void rayTracingParseOptions(
            int *argc,
            char **argv,
            char raytracingMethodsStringOut[],
            char *rayTracerNameOut);
    static void galerkinParseOptions(int *argc, char **argv);
    static void mainForceOneSidedOption(OptionValueWrapper value);
    static void mainMonochromeOption(OptionValueWrapper value);
    static void commandLineImageWidthOption(OptionValueWrapper value);
    static void commandLineImageHeightOption(OptionValueWrapper value);
    static void cameraSetEyePositionOption(OptionValueWrapper val);
    static void cameraSetLookPositionOption(OptionValueWrapper val);
    static void cameraSetUpDirectionOption(OptionValueWrapper val);
    static void cameraSetFieldOfViewOption(OptionValueWrapper val);
    static void iterationMethodOption(OptionValueWrapper value);
    static void hierarchicalOption(OptionValueWrapper value);
    static void lazyOption(OptionValueWrapper value);
    static void clusteringOption(OptionValueWrapper value);
    static void importanceOption(OptionValueWrapper value);
    static void ambientOption(OptionValueWrapper value);
    static void toneMappingMethodOption(OptionValueWrapper value);
    static void brightnessAdjustOption(OptionValueWrapper value);
    static void chromaOption(OptionValueWrapper value);
    static void toneMappingCommandLineOptionDescAdaptMethodOption(OptionValueWrapper value);
    static void gammaOption(OptionValueWrapper value);
    static void flatOption(OptionValueWrapper value);
    static void noCullingOption(OptionValueWrapper value);
    static void outlinesOption(OptionValueWrapper value);
    static void traceOption(OptionValueWrapper value);
    static void binaryOutputOption(OptionValueWrapper value);
    static void binaryInputOption(OptionValueWrapper value);
    static void mainRayTracingOption(OptionValueWrapper value);

  private:
    static constexpr int DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS = 4;
    static constexpr bool DEFAULT_FORCE_ONE_SIDED = true;
    static const Vector3D DEFAULT_CAMERA_EYE_POSITION;
    static const Vector3D DEFAULT_CAMERA_LOOK_POSITION;
    static const Vector3D DEFAULT_CAMERA_UP_DIRECTION;
    static const ColorRgb DEFAULT_BACKGROUND_COLOR;
    static constexpr float DEFAULT_CAMERA_FIELD_OF_VIEW = 22.5f;
    static constexpr int TONE_MAPPING_METHODS_STRING_LENGTH = 1000;

    static int numberOfQuarterCircleDivisions;
    static int fileOptionsForceOneSidedSurfaces;
    static int yesValue;
    static int noValue;
    static int outputImageWidth;
    static int outputImageHeight;
    static int glutDebugEnabled;
    static Camera cameraState;
    static BackgroundMode backgroundMode;
    static ColorRgb backgroundColor;
    static int trueValue;
    static int falseValue;

    static char toneMappingMethodsString[TONE_MAPPING_METHODS_STRING_LENGTH];
    static float redChromaticity[2];
    static float greenChromaticity[2];
    static float blueChromaticity[2];
    static float whiteChromaticity[2];
    static char *toneMapName;
    static ToneMappingContext *toneMapOptions;
    static char *radianceMethodsString;
    static RenderOptions renderOptionsState;
    static ColorRgb outlineColor;
    static BatchOptions batchOptionsState;

#ifdef RAYTRACING_ENABLED
    static EnumDesc approximateValues[];
    static EnumDesc clusteringValues[];
    static EnumDesc sequenceValues[];
    static EnumDesc estimatorTypeValues[];
    static EnumDesc estimatorKindValues[];
    static EnumDesc showWhatValues[];
    static EnumDesc rayMatterPixelFilterValues[];
    static EnumDesc rayTracingRadianceModeValues[];
    static EnumDesc rayTracingLightModeValues[];
    static EnumDesc rayTracingSamplingModeValues[];
    static int regExpStringLength;

    static char *raytracingMethodsString;
    static char *rayTracerName;
#endif

    static bool commandLineParseFloat(const char *text, float *value);
    static bool commandLineParseBackgroundColor(const char *rArg, const char *gArg, const char *bArg, ColorRgb *color);
    static void commandLineParseBackgroundOption(int *argc, char **argv);
    static void cameraDefaults(Camera *camera, int imageWidth, int imageHeight);
    static void makeToneMappingMethodsString();
};

#endif
