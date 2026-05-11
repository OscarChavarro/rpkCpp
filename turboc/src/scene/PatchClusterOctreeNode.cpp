/**
Clusters of patches. A hierarchy of clusters is automatically built after loading a scene.

Patch cluster hierarchies: after loading a scene, an octree hierarchy of patch
clusters is built. Since only patch position and size are relevant for
constructing this hierarchy (and not, for example, material properties or the
original geometry ownership), the generated structure is often more efficient
for ray traversal and clustering-based radiosity routines.

Reference:
- Per Christensen, PhD Thesis "Hierarchical Techniques for Glossy Global Illumination",
  Univ. of Washington, 1995, p116-117 - section 6.7 "Creation of Clusters"
*/

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "skin/Compound.h"
#include "environment/geometry/elements/PatchSet.h"
#include "scene/PatchClusterOctreeNode.h"

ArrayList<Geometry *> *PatchClusterOctreeNode::clusterNodeGeometriesToDelete = NULL;

/**
Creates an empty cluster with initialized bounding box
*/
PatchClusterOctreeNode::PatchClusterOctreeNode() {
    boundingBoxCentroid.set(0.0, 0.0, 0.0);
    patches = new ArrayList<Patch *>();

    for ( int i = 0; i < 8; i++ ) {
        children[i] = NULL;
    }
}

/**
Creates a toplevel cluster. The patch list of the cluster contains all inPatches.
*/
PatchClusterOctreeNode::PatchClusterOctreeNode(const ArrayList<Patch *> *inPatches) {
    boundingBoxCentroid.set(0.0, 0.0, 0.0);

    for ( int i = 0; i < 8; i++ ) {
        children[i] = NULL;
    }

    patches = new ArrayList<Patch *>();
    for ( int i = 0; inPatches != NULL && i < inPatches->size(); i++ ) {
        clusterAddPatch(inPatches->get(i));
    }

    boundingBoxCentroid = boundingBox.center();
}

PatchClusterOctreeNode::~PatchClusterOctreeNode() {
    // Can not delete the list since it is being transferred to geometry...
    for ( int i = 0; i < 8; i++ ) {
        if ( children[i] != NULL ) {
            delete children[i];
            children[i] = NULL;
        }
    }

    delete patches; // Containing just reference to patches owned by external objects
    patches = NULL;
}

void
PatchClusterOctreeNode::addToDeletionCache(Geometry *geometry) {
    if ( clusterNodeGeometriesToDelete == NULL ) {
        clusterNodeGeometriesToDelete = new ArrayList<Geometry *>();
    }
    clusterNodeGeometriesToDelete->add(geometry);
}

void
PatchClusterOctreeNode::deleteCachedGeometries() {
    if ( clusterNodeGeometriesToDelete == NULL ) {
        return;
    }
    for ( int i = 0; i < clusterNodeGeometriesToDelete->size(); i++ ) {
        Geometry *geometry = clusterNodeGeometriesToDelete->get(i);
        if ( geometry != NULL ) {
            geometry->isDuplicate = false;
            geometry->radianceData = NULL; // This was duplicated
            delete geometry;
        }
    }
    delete clusterNodeGeometriesToDelete;
    clusterNodeGeometriesToDelete = NULL;
}

/**
Adds a patch to a cluster. The bounding box is enlarged if necessary, but
the midpoint is not updated (it's more efficient to do that once, after all
patches have been added to the cluster and the bounding box is fully
determined)
*/
void
PatchClusterOctreeNode::clusterAddPatch(Patch *patch) {
    if ( patch != NULL ) {
        patches->add(patch);

        BoundingBox patchBoundingBox = BoundingBox();

        if ( patch->boundingBox != NULL ) {
            patchBoundingBox = *patch->boundingBox;
        } else {
            patch->computeAndGetBoundingBox(&patchBoundingBox);
        }
        boundingBox.enlarge(&patchBoundingBox);
    }
}

/**
- Checks the size of patches[patchIndexOnParent] patch with reference to the bounding box of the cluster
- If the size of the patch is more than half the size of the cluster, the method returns
- If the patch is smaller than half the size of the cluster, the position of
  its centroid is tested with reference to the centroid of the cluster
- If the centroids coincide, the routine returns
- If not, patches[patchIndexOnParent] is moved to the patch list of a sub-cluster of cluster. Which sub-cluster
  depends on the position of the patch with reference to the centroid of cluster
- Returns true if the patch was moved to the sub-cluster
- Returns false if the patch was not moved
- previousClusterNode is the patch list element previous patches[patchIndexOnParent] (chasing pointers!), needed to be
  able to efficiently remove patches[patchIndexOnParent] from the patch list of cluster
*/
bool
PatchClusterOctreeNode::movePatchToSubOctantCluster(const int patchIndexOnParent) const {
    // All patches that were added to the top cluster, which is being split now,
    // have a bounding box computed for them
    Patch *patch = patches->get(patchIndexOnParent);
    const BoundingBox *patchBoundingBox = patch->boundingBox;

    // If the patch is larger than an octant, don´t move current patch from parent to sub-cluster
    float smallestBoxDimension = 10.0f * Numeric::EPSILON_FLOAT;

    if ( (patchBoundingBox->dx() > smallestBoxDimension && patchBoundingBox->dx() > boundingBox.dx() * 0.5f) ||
         (patchBoundingBox->dy() > smallestBoxDimension && patchBoundingBox->dy() > boundingBox.dy() * 0.5f) ||
         (patchBoundingBox->dz() > smallestBoxDimension && patchBoundingBox->dz() > boundingBox.dz() * 0.5f) ) {
        return false;
    }

    // Check the position of the centroid of the bounding box of the patch with reference to the
    // centroid of the cluster
    Vector3D midPatch = patchBoundingBox->center();

    int selectedChildOctantIndex = octantIndex(midPatch);

    // If the centroids (almost by EPSILON) coincides, don´t move current patch from parent cluster to sub-cluster
    if ( selectedChildOctantIndex == XYZ_EQUAL_MASK ) {
        return false;
    }

    // Otherwise, move the patch to the sub cluster with index selectedChildOctantIndex
    PatchClusterOctreeNode *selectedChildCluster = children[selectedChildOctantIndex];

    patches->remove(patchIndexOnParent);
    selectedChildCluster->patches->add(patch);

    // Enlarge the bounding box the of sub-cluster
    selectedChildCluster->boundingBox.enlarge(patchBoundingBox);

    // Current patch was moved to the sub-cluster
    return true;
}

/**
Splits a cluster into sub-clusters: it is assumed that cluster is a cluster without
sub-clusters yet. If cluster is NULL or there are no more than MIN_NUM_O_PTCH_PER_CLST
patches in the cluster, the routine simply returns. If there are more patches, 8
sub-clusters are created for the cluster. Then for each patch, if the patch
is smaller than an octant, the patch is moved to the octant containing its
centroid. Otherwise, the patch remains a direct child of the cluster. Finally,
sub-clusters with zero patches are disposed off and the procedure recursively repeated
for each sub-cluster
*/
void
PatchClusterOctreeNode::splitCluster() {
    // Don't split the cluster if it contains too few patches
    if ( patches != NULL && patches->size() <= MIN_NUM_O_PTCH_PER_CLST ) {
        return;
    }

    // Create eight sub-clusters for the cluster with initialized bounding box
    for ( int i = 0; i < 8; i++ ) {
        children[i] = new PatchClusterOctreeNode();
    }

    // Check and possibly move each of the patches in the cluster to a sub-cluster
    for ( int i = 0; patches != NULL && i < patches->size(); i++ ) {
        if ( movePatchToSubOctantCluster(i) ) {
            i--;
        }
    }

    // Dispose of sub-clusters containing no patches, call splitCluster recursively for not empty sub-clusters
    for ( int i = 0; i < 8; i++ ) {
        if ( children[i]->patches->size() == 0 ) {
            delete children[i];
            children[i] = NULL;
        } else {
            children[i]->boundingBoxCentroid = children[i]->boundingBox.center();
            children[i]->splitCluster();
        }
    }
}

/**
Converts a cluster hierarchy to a regular Geometry tree.
This lets the standard ray traversal code operate on clusters, including
operations such as shaft culling, without specialized traversal code.
*/
Geometry *
PatchClusterOctreeNode::convertClusterToGeometry() const {
    Geometry *parentPatchesGeometry = NULL;
    if ( patches != NULL && patches->size() > 0) {
        parentPatchesGeometry = new PatchSet(patches);
        addToDeletionCache(parentPatchesGeometry);
    }

    ArrayList<Geometry *> *patchesGeometryList = new ArrayList<Geometry *>();

    // The patches in the cluster are the first to be tested for intersection with
    if ( parentPatchesGeometry != NULL ) {
        patchesGeometryList->add(parentPatchesGeometry);
    }

    for ( int i = 0; i < 8; i++ ) {
        Geometry *child = NULL;
        if ( children[i] != NULL ) {
            child = children[i]->convertClusterToGeometry();
        }

        if ( child != NULL ) {
            patchesGeometryList->add(child);
        }
    }

    Geometry *newGeometry = new Compound(patchesGeometryList);
    addToDeletionCache(newGeometry);
    return newGeometry;
}

void
PatchClusterOctreeNode::print(const int level) const {
    switch ( level ) {
        case 0:
            System::out.printf("= PatchClusterOctreeNode ================================================================\n");
            break;
        case 1:
            System::out.printf("  - ");
            break;
        case 2:
            System::out.printf("    . ");
            break;
        default:
            System::out.printf("      (%d) ", level);
            for ( int i = 3; i < level; i++ ) {
                System::out.printf(" ");
            }
            System::out.printf("-> ");
            break;
    }
    System::out.printf("%ld patches: ", patches->size());
    for ( int i = 0; i < patches->size(); i++ ) {
        System::out.printf("[%d]", patches->get(i)->id);
    }
    System::out.printf("\n");
    for ( int i = 0; i < 8; i++ ) {
        if ( children[i] != NULL ) {
            children[i]->print(level + 1);
        }
    }
}
