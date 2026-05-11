#ifndef RADIANCE_OPTIONS_GROUP__
#define RADIANCE_OPTIONS_GROUP__

#include "vsdk/toolkit/scene/RadianceMethod.h"

#ifdef RAYTRACING_ENABLED
    #include "vsdk/toolkit/raycasting/photonMap/PhotonMapConfig.h"
    #include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"
    #include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
    #include "vsdk/toolkit/raycasting/simple/RayMatterState.h"
    #include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"
    #include "vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState.h"
    #include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
    #include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"
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
