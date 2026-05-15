#ifndef __RADIANCE__
#define __RADIANCE__

#include "raycasting/photonMap/PhotonMapConfig.h"
#include "raycasting/photonMap/PhotonMapState.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "raycasting/simple/RayMatterState.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "scene/RadianceMethod.h"
#include "tonemap/ToneMappingContext.h"

class Radiance{ public:
    static void radianceParseOptions( int *argc, char **argv, RadianceMethod **newRadianceMethod, StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState, PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig, RayMatterState &rayMatterState, BidirectionalPathTracingState &bidirectionalPathState, StochasticRayTracingState &stochasticRayTracingState);
    static void setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene, ToneMappingContext *toneMapOptions);
};

#endif
