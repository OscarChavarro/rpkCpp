#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

#include "raycasting/common/RayTracer.h"
#include "common/RenderOptions.h"
#include "app/options/BatchOptions.h"
#include "app/options/BackgroundMode.h"
#include "app/options/EnumDesc.h"
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
    static void mainForceOneSidedOption(int &value);
    static void mainMonochromeOption(int &value);
    static void commandLineImageWidthOption(int &value);
    static void commandLineImageHeightOption(int &value);
    static void cameraSetEyePositionOption(Vector3D &val);
    static void cameraSetLookPositionOption(Vector3D &val);
    static void cameraSetUpDirectionOption(Vector3D &val);
    static void cameraSetFieldOfViewOption(float &val);
    static void iterationMethodOption(char *&value);
    static void hierarchicalOption(int &value);
    static void lazyOption(int &value);
    static void clusteringOption(int &value);
    static void importanceOption(int &value);
    static void ambientOption(int &value);
    static void toneMappingMethodOption(char *&value);
    static void brightnessAdjustOption(float &value);
    static void redChromaOption(Vector3D &value);
    static void greenChromaOption(Vector3D &value);
    static void blueChromaOption(Vector3D &value);
    static void whiteChromaOption(Vector3D &value);
    static void toneMappingCommandLineOptionDescAdaptMethodOption(char *&value);
    static void gammaOption(float &value);
    static void flatOption(int &value);
    static void noCullingOption(int &value);
    static void outlinesOption(int &value);
    static void traceOption(int &value);
    static void binaryOutputOption(const char *&value);
    static void binaryInputOption(const char *&value);
    static void mainRayTracingOption(char *&value);

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
