#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __OPTIONS_GROUP_RADIANCE__
#define __OPTIONS_GROUP_RADIANCE__

#include "vsdk/toolkit/raycasting/photonMap/PhotonMapConfig.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "vsdk/toolkit/raycasting/simple/RayMatterState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"

class OptionsGroupRadiance{ public:
    static void parse( int *argc, char **argv, RadianceMethod **newRadianceMethod, StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState, PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig, RayMatterState &rayMatterState, BidirectionalPathTracingState &bidirectionalPathState, StochasticRayTracingState &stochasticRayTracingState);

  private:
    #define RADIANCE_METHODS_STRING_LENGTH 1000

    static void selectRadianceMethod( const char *name, RadianceMethod **newRadianceMethod, StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState, PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig);
};

#endif
