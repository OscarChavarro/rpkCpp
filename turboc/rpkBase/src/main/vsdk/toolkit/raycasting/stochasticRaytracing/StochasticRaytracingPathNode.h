#ifndef STCHS_RYTRC_PATH_NODE
#define STCHS_RYTRC_PATH_NODE

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

/**
Path node: contains all necessary data for computing the score afterwards
*/
class StochasticRaytracingPathNode {
  public:
    Patch *patch;
    double probability;
    Vector3Df inPoint;
    Vector3Df outpoint;

    StochasticRaytracingPathNode();
};

#endif
