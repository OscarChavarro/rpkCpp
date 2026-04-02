#ifndef __RADIANCE__
#define __RADIANCE__

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

class Radiance final {
  public:
    static void radianceParseOptions(
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
    static void setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene);
};

#endif
