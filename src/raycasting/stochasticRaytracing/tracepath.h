/**
Random walk generation
*/

#ifndef __TRACE_PATH__
#define __TRACE_PATH__

#include "java/util/ArrayList.h"

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

/**
A full path, basically an array of 'numberOfNodes' path nodes
*/
class PATH {
  public:
    int numberOfNodes;
    int nodesAllocated;
    StochasticRaytracingPathNode *nodes;

    PATH(): numberOfNodes(), nodesAllocated(), nodes() {}
};

extern void
tracePaths(
    long numberOfPaths,
    double (*BirthProbability)(Patch *P),
    double (*SurvivalProbability)(Patch *P),
    void (*ScorePath)(PATH *, long nr_paths, double (*birth_prob)(Patch *)),
    void (*Update)(Patch *P, double w),
    java::ArrayList<Patch *> *scenePatches);

#endif
