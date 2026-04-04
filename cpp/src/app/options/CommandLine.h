#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

#include "raycasting/common/RayTracer.h"
#include "app/options/EnumDesc.h"
#include "raycasting/photonMap/PhotonMapState.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

class CommandLine final {
  public:
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

  private:
    template<typename T>
    class EnumBinding {
      public:
        T *target;
        const EnumDesc *values;
    };

    class FixedStringBinding {
      public:
        char *target;
        int maxLength;
    };

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

    template<typename T>
    static bool parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding);
    static bool parseFixedStringBinding(int argc, char **argv, FixedStringBinding &binding);
    static bool parseBoolInt(int argc, char **argv, int &value);
    static void setIntTrue(int &value);
    static void setIntFalse(int &value);
};

#endif
