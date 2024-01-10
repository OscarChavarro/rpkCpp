/* tracepath.h: random walk generation */

#ifndef _TRACEPATH_H_
#define _TRACEPATH_H_

/* path node: contains all necessary data for computing the score afterwards */
class PATHNODE {
  public:
    Patch *patch;
    double probability;
    Vector3D inpoint, outpoint;
};

/* a full path, basically an array of 'numberOfNodes' PATHNODEs */
class PATH {
  public:
    int nrnodes;
    int nodesalloced;
    PATHNODE *nodes;
};

extern void tracePaths(long numberOfPaths,
                       double (*BirthProbability)(Patch *P),
                       double (*SurvivalProbability)(Patch *P),
                       void (*ScorePath)(PATH *, long nr_paths, double (*birth_prob)(Patch *)),
                       void (*Update)(Patch *P, double w));

#endif
