#ifndef __RADIANCE_OPTIONS_PARSER__
#define __RADIANCE_OPTIONS_PARSER__

#include "app/options/OptionsType.h"
#include "photonMap/PhotonMapConfig.h"
#include "photonMap/PhotonMapState.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "scene/RadianceMethod.h"

class RadianceOptionsParser final {
  public:
    static void parse(
        int *argc,
        char **argv,
        RadianceMethod **newRadianceMethod,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState,
        OptionsType &optionTypes);

  private:
    static constexpr int RADIANCE_METHODS_STRING_LENGTH = 1000;

    static void selectRadianceMethod(
        const int *argc,
        char **argv,
        RadianceMethod **newRadianceMethod,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig);
};

#endif
