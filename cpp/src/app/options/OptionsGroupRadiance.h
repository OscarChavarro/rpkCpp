#ifndef RADIANCE_OPTIONS_GROUP__
#define RADIANCE_OPTIONS_GROUP__

#include "scene/RadianceMethod.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/photonMap/PhotonMapConfig.h"
    #include "raycasting/photonMap/PhotonMapState.h"
    #include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
    #include "raycasting/simple/RayMatterState.h"
    #include "raycasting/stochasticRaytracing/Basismcrad.h"
    #include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
    #include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
    #include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
#endif

class OptionsGroupRadiance final {
  public:
    static void parse(
        int *argc,
        char **argv,
        RadianceMethod **newRadianceMethod
#ifdef RAYTRACING_ENABLED
        ,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState
#endif
        );

  private:
    static constexpr int RADIANCE_METHODS_STRING_LENGTH = 1000;

    static void selectRadianceMethod(
        const char *name,
        RadianceMethod **newRadianceMethod
#ifdef RAYTRACING_ENABLED
        ,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig
#endif
        );
};

#endif
