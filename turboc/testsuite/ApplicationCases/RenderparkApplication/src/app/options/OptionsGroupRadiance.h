#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __RADIANCE_OPTIONS_GROUP__
#define __RADIANCE_OPTIONS_GROUP__

#include "vsdk/raycasting/photonMap/PhotonMapConfig.h"
#include "vsdk/raycasting/photonMap/PhotonMapState.h"
#include "vsdk/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
#include "vsdk/raycasting/simple/RayMatterState.h"
#include "vsdk/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "vsdk/raycasting/stochasticRaytracing/StochasticRayTracingState.h"
#include "vsdk/raycasting/stochasticRaytracing/StochasticRelaxation.h"
#include "vsdk/scene/RadianceMethod.h"

class OptionsGroupRadiance{ public:
    static void parse( int *argc, char **argv, RadianceMethod **newRadianceMethod, StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState, PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig, RayMatterState &rayMatterState, BidirectionalPathTracingState &bidirectionalPathState, StochasticRayTracingState &stochasticRayTracingState);

  private:
    #define RADIANCE_METHODS_STRING_LENGTH 1000

    static void selectRadianceMethod( const char *name, RadianceMethod **newRadianceMethod, StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState, PhotonMapState &photonMapState, PhotonMapConfig &photonMapConfig);
};

#endif
