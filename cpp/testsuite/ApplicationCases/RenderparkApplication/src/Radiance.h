#ifndef RADIANCE__
#define RADIANCE__

#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

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

class Radiance final {
  public:
    static void radianceParseOptions(
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
    static void setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene, ToneMappingContext *toneMapOptions);
};

#endif
