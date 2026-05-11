#ifndef RADIANCE__
#define RADIANCE__

#include "scene/RadianceMethod.h"
#include "tonemap/ToneMappingContext.h"

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
