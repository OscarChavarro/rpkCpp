/* tracepath.h: random walk generation */

#ifndef _TRACEPATH_H_
#define _TRACEPATH_H_

/* path node: contains all necessary data for computing the score afterwards */
class PATHNODE {
  public:
    PATCH *patch;
    double probability;
    Vector3D inpoint, outpoint;
};

/* a full path, basically an array of 'nrnodes' PATHNODEs */
class PATH {
  public:
    int nrnodes;
    int nodesalloced;
    PATHNODE *nodes;
};

extern void tracePaths(long numberOfPaths,
                       double (*BirthProbability)(PATCH *P),
                       double (*SurvivalProbability)(PATCH *P),
                       void (*ScorePath)(PATH *, long nr_paths, double (*birth_prob)(PATCH *)),
                       void (*Update)(PATCH *P, double w));

#endif
