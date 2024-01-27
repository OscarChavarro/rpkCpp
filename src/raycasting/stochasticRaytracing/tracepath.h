/**
Random walk generation
*/

#ifndef __TRACE_PATH__
#define __TRACE_PATH__

#include "java/util/ArrayList.h"

/**
Path node: contains all necessary data for computing the score afterwards
*/
class PATHNODE {
  public:
    Patch *patch;
    double probability;
    Vector3D inpoint, outpoint;
};

/**
A full path, basically an array of 'numberOfNodes' PATHNODEs
*/
class PATH {
  public:
    int nrnodes;
    int nodesalloced;
    PATHNODE *nodes;
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
