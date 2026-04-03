#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

#include "raycasting/common/RayTracer.h"
#include "common/RenderOptions.h"
#include "app/options/BatchOptions.h"
#include "app/options/BackgroundMode.h"
#include "app/options/EnumDesc.h"
#include "app/options/CommandLineOptions.h"
#include "app/options/OptionsType.h"
#include "photonMap/PhotonMapState.h"
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
            int imageHeight,
            OptionsType &optionTypes);
    static void commandLineGeneralProgramParseOptions(
            int *argc,
            char **argv,
            bool *oneSidedSurfaces,
            int *conicSubDivisions,
            int *imageOutputWidth,
            int *imageOutputHeight,
            bool *glutDebugEnabledOut,
            OptionsType &optionTypes);
    static void stochasticRelaxationRadiosityParseOptions(
            int *argc,
            char **argv,
            StochasticRelaxation &stochasticRelaxationState,
            ElementHierarchyState &elementHierarchyState,
            OptionsType &optionTypes);
    static void randomWalkRadiosityParseOptions(
            int *argc,
            char **argv,
            StochasticRelaxation &stochasticRelaxationState,
            OptionsType &optionTypes);
    static void rayMattingParseOptions(
            int *argc,
            char **argv,
            RayMatterState &rayMatterState,
            OptionsType &optionTypes);
    static void stochasticRayTracerParseOptions(
            int *argc,
            char **argv,
            StochasticRayTracingState &stochasticRayTracingState,
            OptionsType &optionTypes);
    static void biDirectionalPathParseOptions(
            int *argc,
            char **argv,
            BidirectionalPathTracingState &bidirectionalPathState,
            OptionsType &optionTypes);
    static void photonMapParseOptions(
            int *argc,
            char **argv,
            PhotonMapState &photonMapState,
            OptionsType &optionTypes);
    static void toneMapParseOptions(
            int *argc,
            char **argv,
            char *toneMapNameOut,
            ToneMappingContext &toneMapOptionsContext,
            OptionsType &optionTypes);
    static void radianceMethodParseOptions(
            int *argc,
            char **argv,
            char *radianceMethodsStringOut,
            OptionsType &optionTypes);
    static void renderParseOptions(
            int *argc,
            char **argv,
            RenderOptions *renderOptions,
            OptionsType &optionTypes);
    static void batchParseOptions(
            int *argc,
            char **argv,
            BatchOptions *batchOptions,
            OptionsType &optionTypes);
    static void rayTracingParseOptions(
            int *argc,
            char **argv,
            char raytracingMethodsStringOut[],
            char *rayTracerNameOut,
            OptionsType &optionTypes);
    static void galerkinParseOptions(int *argc, char **argv, OptionsType &optionTypes);
    static void mainForceOneSidedOption(void *value);
    static void mainMonochromeOption(void *value);
    static void commandLineImageWidthOption(void *value);
    static void commandLineImageHeightOption(void *value);
    static void cameraSetEyePositionOption(void *val);
    static void cameraSetLookPositionOption(void *val);
    static void cameraSetUpDirectionOption(void *val);
    static void cameraSetFieldOfViewOption(void *val);
    static void iterationMethodOption(void *value);
    static void hierarchicalOption(void *value);
    static void lazyOption(void *value);
    static void clusteringOption(void *value);
    static void importanceOption(void *value);
    static void ambientOption(void *value);
    static void toneMappingMethodOption(void *value);
    static void brightnessAdjustOption(void *value);
    static void chromaOption(void *value);
    static void toneMappingCommandLineOptionDescAdaptMethodOption(void *value);
    static void gammaOption(void *value);
    static void flatOption(void *value);
    static void noCullingOption(void *value);
    static void outlinesOption(void *value);
    static void traceOption(void *value);
    static void binaryOutputOption(void *value);
    static void binaryInputOption(void *value);
    static void mainRayTracingOption(void *value);

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
    static CommandLineOptions approximationTypeOption;

    static EnumDesc clusteringValues[];
    static CommandLineOptions clusteringTypeOption;

    static EnumDesc sequenceValues[];
    static CommandLineOptions sequenceTypeOption;

    static EnumDesc estimatorTypeValues[];
    static CommandLineOptions estimatorTypeOption;

    static EnumDesc estimatorKindValues[];
    static CommandLineOptions estimatorKindTypeOption;

    static EnumDesc showWhatValues[];
    static CommandLineOptions showWhatTypeOption;

    static EnumDesc rayMatterPixelFilterValues[];
    static CommandLineOptions rayMatterPixelFilterTypeOption;

    static EnumDesc rayTracingRadianceModeValues[];
    static CommandLineOptions rayTracingRadianceModeTypeOption;

    static EnumDesc rayTracingLightModeValues[];
    static CommandLineOptions rayTracingLightModeTypeOption;

    static EnumDesc rayTracingSamplingModeValues[];
    static CommandLineOptions rayTracingSamplingModeTypeOption;

    static CommandLineOptions regExpStringType;

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
