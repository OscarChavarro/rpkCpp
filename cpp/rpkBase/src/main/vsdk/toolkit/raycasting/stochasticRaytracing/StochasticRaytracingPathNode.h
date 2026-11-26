#ifndef __STOCHASTIC_RAYTRACING_PATH_NODE__
#define __STOCHASTIC_RAYTRACING_PATH_NODE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

/**
Path node: contains all necessary data for computing the score afterwards
*/
class StochasticRaytracingPathNode {
  public:
    const Patch *patch;
    double probability;
    Vector3Dd inPoint;
    Vector3Dd outpoint;

    StochasticRaytracingPathNode();
};

#endif
