#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

#include "raycasting/common/RayTracer.h"
#include "app/BatchOptions.h"
class ToneMappingContext;
class RayMatterState;
class BidirectionalPathTracingState;
class StochasticRayTracingState;
class StochasticRelaxation;
class ElementHierarchyState;
class PhotonMapState;
class OptionsType;

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
            bool *glutDebugEnabled,
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
            char *toneMapName,
            ToneMappingContext &toneMapOptions,
            OptionsType &optionTypes);
    static void radianceMethodParseOptions(
            int *argc,
            char **argv,
            char *radianceMethodsString,
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
            char raytracingMethodsString[],
            char *rayTracerName,
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
    static bool commandLineParseFloat(const char *text, float *value);
    static bool commandLineParseBackgroundColor(const char *rArg, const char *gArg, const char *bArg, ColorRgb *color);
    static void commandLineParseBackgroundOption(int *argc, char **argv);
    static void cameraDefaults(Camera *camera, int imageWidth, int imageHeight);
    static void makeToneMappingMethodsString();
};

#endif
