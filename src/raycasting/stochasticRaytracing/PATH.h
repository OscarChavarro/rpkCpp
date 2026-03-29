#ifndef __STOCHASTIC_RAYTRACING_PATH__
#define __STOCHASTIC_RAYTRACING_PATH__

#include "raycasting/stochasticRaytracing/StochasticRaytracingPathNode.h"

/**
A full path, basically an array of 'numberOfNodes' path nodes
*/
class PATH {
  public:
    int numberOfNodes;
    int nodesAllocated;
    StochasticRaytracingPathNode *nodes;

    PATH();
};

inline
PATH::PATH(): numberOfNodes(), nodesAllocated(), nodes() {
}

#endif
