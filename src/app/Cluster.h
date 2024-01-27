#ifndef __CLUSTER__
#define __CLUSTER__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class Cluster {
private:
    java::ArrayList<Patch *> *patches;
    Cluster *children[8]{}; // Clusters form an octree
    BOUNDINGBOX boundingBox{}; // Bounding box for the cluster
    Vector3D boundingBoxCentroid; // Mid-point of the bounding box

    Cluster();
    void commonBuild();
    bool clusterMovePatch(int parentIndex);
    void clusterAddPatch(Patch *patch);

public:
    virtual ~Cluster();
    explicit Cluster(java::ArrayList<Patch *> *inPatches);
    void splitCluster();
    Geometry * convertClusterToGeom();
};

#endif
