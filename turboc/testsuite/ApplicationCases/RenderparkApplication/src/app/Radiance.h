#ifndef __RADIANCE__
#define __RADIANCE__

#include "vsdk/raycasting/photonMap/PhotonMapConfig.h"
#include "vsdk/raycasting/photonMap/PhotonMapState.h"
#include "vsdk/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "vsdk/raycasting/simple/RayMatterState.h"
#include "vsdk/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "vsdk/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "vsdk/raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "vsdk/scene/RadianceMethod.h"
#include "vsdk/tonemap/ToneMappingContext.h"

class Radiance{ public:
    static void radianceParseOptions( int *argc, char **argv, RadianceMethod **newRadianceMethod, StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState, PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig, RayMatterState &rayMatterState, BidirectionalPathTracingState &bidirectionalPathState, StochasticRayTracingState &stochasticRayTracingState);
    static void setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene, ToneMappingContext *toneMapOptions);
};

#endif
