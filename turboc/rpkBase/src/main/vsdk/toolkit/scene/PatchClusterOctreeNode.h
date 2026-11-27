#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __PATCH_CLUSTER_OCTREE_NODE__
#define __PATCH_CLUSTER_OCTREE_NODE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"

class PatchClusterOctreeNode {
  private:
    #define XYZ_EQUAL_MASK 0x08
    // No clusters are created with less than this number of patches.
    #define MIN_NUM_O_PTCH_PER_CLST 3

    ArrayList<Patch *> *patches;
    PatchClusterOctreeNode *children[8]; // Clusters form an octree
    RpkAxisAlignedBoundingBox boundingBox; // Bounding box for the cluster
    Vector3Dd boundingBoxCentroid; // Mid-point of the bounding box
    static ArrayList<RpkGeometry *> *clusterNodeGeometriesToDelete;
    static void addToDeletionCache(RpkGeometry *geometry);

    PatchClusterOctreeNode();
    bool movePatchToSubOctantCluster(int patchIndexOnParent) const;
    void clusterAddPatch(Patch *patch);
    int octantIndex(const Vector3Dd &p) const;

  public:
    explicit PatchClusterOctreeNode(const ArrayList<Patch *> *inPatches);
    virtual ~PatchClusterOctreeNode();

    void splitCluster();
    RpkGeometry *convertClusterToGeometry() const;
    static void deleteCachedGeometries();
    void print(int level) const;
};

inline int
PatchClusterOctreeNode::octantIndex(const Vector3Dd &p) const {
    Vector3Dd c = boundingBox.center();
    int index = 0;

    if (p.x() > c.x()) {
        index |= 1;
    }

    if (p.y() > c.y()) {
        index |= 2;
    }

    if (p.z() > c.z()) {
        index |= 4;
    }

    return index;
}

#endif
