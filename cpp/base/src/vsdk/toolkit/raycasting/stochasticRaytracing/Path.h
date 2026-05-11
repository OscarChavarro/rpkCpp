#ifndef STOCHASTIC_RAYTRACING_PATH__
#define STOCHASTIC_RAYTRACING_PATH__

#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingPathNode.h"

/**
A full path, basically an array of 'numberOfNodes' path nodes
*/
class Path {
  public:
    int numberOfNodes;
    int nodesAllocated;
    StochasticRaytracingPathNode *nodes;

    Path();
};

inline
Path::Path(): numberOfNodes(), nodesAllocated(), nodes() {
}

#endif
