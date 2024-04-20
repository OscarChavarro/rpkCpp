#ifndef __CLUSTER__
#define __CLUSTER__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class Cluster {
  private:
    java::ArrayList<Patch *> *patches;
    Cluster *children[8]{}; // Clusters form an octree
    BoundingBox boundingBox{}; // Bounding box for the cluster
    Vector3D boundingBoxCentroid; // Mid-point of the bounding box
    static java::ArrayList<Geometry *> *clusterNodeGeometriesToDelete;
    static void addToDeletionCache(Geometry *geometry);

    Cluster();
    void commonBuild();
    bool clusterMovePatch(int parentIndex);
    void clusterAddPatch(Patch *patch);

  public:
    explicit Cluster(java::ArrayList<Patch *> *inPatches);
    virtual ~Cluster();

    void splitCluster();
    Geometry *convertClusterToGeometry();
    static void deleteCachedGeometries();
};

#endif
