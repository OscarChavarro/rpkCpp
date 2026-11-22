#ifndef STOCHASTIC_RAYTRACING_PATH_NODE__
#define STOCHASTIC_RAYTRACING_PATH_NODE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

/**
Path node: contains all necessary data for computing the score afterwards
*/
class StochasticRaytracingPathNode {
  public:
    const Patch *patch;
    double probability;
    Vector3Df inPoint;
    Vector3Df outpoint;

    StochasticRaytracingPathNode();
};

#endif
