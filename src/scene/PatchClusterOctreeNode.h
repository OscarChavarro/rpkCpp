#ifndef __CLUSTER__
#define __CLUSTER__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"

class PatchClusterOctreeNode {
  private:
    java::ArrayList<Patch *> *patches;
    PatchClusterOctreeNode *children[8]{}; // Clusters form an octree
    BoundingBox boundingBox{}; // Bounding box for the cluster
    Vector3D boundingBoxCentroid; // Mid-point of the bounding box
    static java::ArrayList<Geometry *> *clusterNodeGeometriesToDelete;
    static void addToDeletionCache(Geometry *geometry);

    PatchClusterOctreeNode();
    bool movePatchToSubOctantCluster(int patchIndexOnParent);
    void clusterAddPatch(Patch *patch);

  public:
    explicit PatchClusterOctreeNode(const java::ArrayList<Patch *> *inPatches);
    virtual ~PatchClusterOctreeNode();

    void splitCluster();
    Geometry *convertClusterToGeometry();
    static void deleteCachedGeometries();
    void print(int level) const;
};

#endif
