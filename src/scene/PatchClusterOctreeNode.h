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
    bool movePatchToSubOctantCluster(int patchIndexOnParent) const;
    void clusterAddPatch(Patch *patch);
    int octantIndex(const Vector3D &p) const;

  public:
    explicit PatchClusterOctreeNode(const java::ArrayList<Patch *> *inPatches);
    virtual ~PatchClusterOctreeNode();

    void splitCluster();
    Geometry *convertClusterToGeometry() const;
    static void deleteCachedGeometries();
    void print(int level) const;
};

inline int
PatchClusterOctreeNode::octantIndex(const Vector3D &p) const {
    Vector3D c = boundingBox.center();
    int index = 0;

    if (p.x > c.x) {
        index |= 1;
    }

    if (p.y > c.y) {
        index |= 2;
    }

    if (p.z > c.z) {
        index |= 4;
    }

    return index;
}

#endif
