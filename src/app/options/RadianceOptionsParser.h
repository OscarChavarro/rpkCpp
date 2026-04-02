#ifndef __RADIANCE_OPTIONS_PARSER__
#define __RADIANCE_OPTIONS_PARSER__

#include "scene/RadianceMethod.h"

class RayMatterState;
class BidirectionalPathTracingState;
class StochasticRayTracingState;
class StochasticRelaxation;
class ElementHierarchyState;
class StochasticRadiosityBasisState;
class PhotonMapState;
class PhotonMapConfig;
class OptionsType;

class RadianceOptionsParser final {
  public:
    static void parse(
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

  private:
    static constexpr int RADIANCE_METHODS_STRING_LENGTH = 1000;

    static void selectRadianceMethod(
        const int *argc,
        char **argv,
        RadianceMethod **newRadianceMethod,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState,
        PhotonMapState &photonMapState,
        PhotonMapConfig &photonMapConfig);
};

#endif
