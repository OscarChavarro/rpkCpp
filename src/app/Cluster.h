#ifndef __CLUSTER__
#define __CLUSTER__

#include "java/util/ArrayList.h"
#include "skin/geom.h"

class Cluster {
private:
    java::ArrayList<PATCH *> *patches;
    Cluster *children[8]{}; // Clusters form an octree
    BOUNDINGBOX boundingBox{}; // Bounding box for the cluster
    Vector3D boundingBoxCentroid; // Mid-point of the bounding box

    Cluster();
    bool clusterNewCheckMovePatch(int parentIndex);
    void clusterAddPatch(PATCH *patch);

public:
    virtual ~Cluster();
    explicit Cluster(PATCHLIST *inPatches);
    void splitCluster();
    GEOM * convertClusterToGeom();
};

#endif
