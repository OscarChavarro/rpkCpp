#ifndef __RADIANCE__
#define __RADIANCE__

#include "vsdk/toolkit/raycasting/photonMap/PhotonMapConfig.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "vsdk/toolkit/raycasting/simple/RayMatterState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

class Radiance{ public:
    static void radianceParseOptions( int *argc, char **argv, RadianceMethod **newRadianceMethod, StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState, PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig, RayMatterState &rayMatterState, BidirectionalPathTracingState &bidirectionalPathState, StochasticRayTracingState &stochasticRayTracingState);
    static void setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene, ToneMappingContext *toneMapOptions);
};

#endif
