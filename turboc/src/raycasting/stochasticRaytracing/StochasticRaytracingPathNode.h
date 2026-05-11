#ifndef STCHS_RYTRC_PATH_NODE
#define STCHS_RYTRC_PATH_NODE

#include "common/linealAlgebra/Vector3D.h"
#include "environment/geometry/elements/Patch.h"

/**
Path node: contains all necessary data for computing the score afterwards
*/
class StochasticRaytracingPathNode {
  public:
    Patch *patch;
    double probability;
    Vector3D inPoint;
    Vector3D outpoint;

    StochasticRaytracingPathNode();
};

#endif
