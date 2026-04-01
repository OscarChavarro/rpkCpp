#ifndef __RADIANCE__
#define __RADIANCE__

#include "scene/RadianceMethod.h"

class RayMatterState;
class BidirectionalPathTracingState;
class StochasticRayTracingState;
class StochasticRelaxation;
class ElementHierarchyState;
class StochasticRadiosityBasisState;

class Radiance final {
  public:
    static void radianceParseOptions(
        int *argc,
        char **argv,
        RadianceMethod **newRadianceMethod,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState,
        RayMatterState &rayMatterState,
        BidirectionalPathTracingState &bidirectionalPathState,
        StochasticRayTracingState &stochasticRayTracingState);
    static void setRadianceMethod(RadianceMethod *radianceMethod, Scene *scene);

  private:
    static void selectRadianceMethod(
        const int *argc,
        char **argv,
        RadianceMethod **newRadianceMethod,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState);
};

#endif
