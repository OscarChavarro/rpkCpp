/**
Clusters of patches. A hierarchy of clusters is automatically built after loading a scene.

Patch cluster hierarchies: after loading a scene, a hierarchy of clusters
containing patches is built. Since only the position and size of the patches is
relevant for constructing this GLOBAL_stochasticRaytracing_hierarchy (and not e.g. the material of the patches or
the geometry to which they belong ...), this automatically constructed GLOBAL_stochasticRaytracing_hierarchy
is often more efficient for tracing rays and for use in a clustering radiosity method etc.

Reference:
- Per Christensen, PhD Thesis "Hierarchical Techniques for Glossy Global Illumination",
  Univ. of Washington, 1995, p116-117 - section 6.7 "Creation of Clusters"
*/

#include "java/util/ArrayList.txx"
#include "skin/Compound.h"
#include "scene/Cluster.h"

// No clusters are created with less than this number of patches
#define MINIMUM_NUMBER_OF_PATCHES_PER_CLUSTER 3

java::ArrayList<Geometry *> *Cluster::clusterNodeGeometriesToDelete = nullptr;

/**
Creates an empty cluster with initialized bounding box
*/
Cluster::Cluster(): patches() {
    commonBuild();
}

/**
Creates a toplevel cluster. The patch list of the cluster contains all inPatches.
*/
Cluster::Cluster(java::ArrayList<Patch *> *inPatches) {
    patches = inPatches;
    commonBuild();

    patches = new java::ArrayList<Patch *>();
    for ( int i = 0; inPatches != nullptr && i < inPatches->size(); i++ ) {
        clusterAddPatch(inPatches->get(i));
    }

    boundingBoxCentroid.set(
        (boundingBox.coordinates[MIN_X] + boundingBox.coordinates[MAX_X]) * 0.5f,
        (boundingBox.coordinates[MIN_Y] + boundingBox.coordinates[MAX_Y]) * 0.5f,
        (boundingBox.coordinates[MIN_Z] + boundingBox.coordinates[MAX_Z]) * 0.5f);
}

Cluster::~Cluster() {
    // Can not delete the list since it is being transferred to geometry...
    for ( int i = 0; i < 8; i++ ) {
        if ( children[i] != nullptr ) {
            delete children[i];
            children[i] = nullptr;
        }
    }

    delete patches; // Containing just reference to patches owned by external objects
    patches = nullptr;
}

void
Cluster::addToDeletionCache(Geometry *geometry) {
    if ( clusterNodeGeometriesToDelete == nullptr ) {
        clusterNodeGeometriesToDelete = new java::ArrayList<Geometry *>();
    }
    clusterNodeGeometriesToDelete->add(geometry);
}

void
Cluster::deleteCachedGeometries() {
    if ( clusterNodeGeometriesToDelete == nullptr ) {
        return;
    }
    for ( int i = 0; i < clusterNodeGeometriesToDelete->size(); i++ ) {
        Geometry *geometry = clusterNodeGeometriesToDelete->get(i);
        if ( geometry != nullptr ) {
            geometry->isDuplicate = false;
            geometry->radianceData = nullptr; // This was duplicated
            delete geometry;
        }
    }
    delete clusterNodeGeometriesToDelete;
    clusterNodeGeometriesToDelete = nullptr;
}

void
Cluster::commonBuild() {
    boundingBoxCentroid.set(0.0, 0.0, 0.0);

    if ( patches == nullptr ) {
        patches = new java::ArrayList<Patch *>();
    }

    for ( int i = 0; i < 8; i++ ) {
        children[i] = nullptr;
    }
}

/**
Adds a patch to a cluster. The bounding box is enlarged if necessary, but
the midpoint is not updated (it's more efficient to do that once, after all
patches have been added to the cluster and the bounding box is fully
determined)
*/
void
Cluster::clusterAddPatch(Patch *patch) {
    BoundingBox patchBoundingBox{};

    if ( patch != nullptr ) {
        patches->add(patch);
        if ( patch->boundingBox != nullptr ) {
            patchBoundingBox = *patch->boundingBox;
        } else {
            patch->getBoundingBox(&patchBoundingBox);
        }
        boundingBox.enlarge(&patchBoundingBox);
    }
}

/**
- Checks the size of parentClusterNode->patch with reference to the bounding box of the cluster cluster.
- If the size of the patch is more than half the size of the cluster, the routine
  returns.
- If the patch is smaller than half the size of the cluster, the position of
  its centroid is tested with reference to the centroid of the cluster.
- If the centroids coincide, the routine returns.
- If not, parentClusterNode is moved to the patch list of a sub-cluster of cluster. Which sub-cluster
  depends on the position of the patch with reference to the centroid of cluster.
- Returns true if the patch was moved to the sub-cluster.
- Returns false if the patch was not moved.
- previousClusterNode is the patch list element previous parentClusterNode (chasing pointers!), needed to be
  able to efficiently remove parentClusterNode from the patch list of cluster
*/
bool
Cluster::clusterMovePatch(int parentIndex) {
    // All patches that were added to the top cluster, which is being split now,
    // have a bounding box computed for them
    Patch *patch = patches->get(parentIndex);
    BoundingBox *patchBoundingBox = patch->boundingBox;

    // If the patch is larger than an octant, don´t move current patch from parent to sub-cluster
    float smallestBoxDimension = 10 * EPSILON;
    float dx = patchBoundingBox->coordinates[MAX_X] - patchBoundingBox->coordinates[MIN_X];
    float dy = patchBoundingBox->coordinates[MAX_Y] - patchBoundingBox->coordinates[MIN_Y];
    float dz = patchBoundingBox->coordinates[MAX_Z] - patchBoundingBox->coordinates[MIN_Z];

    if ((dx > smallestBoxDimension && dx > (boundingBox.coordinates[MAX_X] - boundingBox.coordinates[MIN_X]) * 0.5) ||
        (dy > smallestBoxDimension && dy > (boundingBox.coordinates[MAX_Y] - boundingBox.coordinates[MIN_Y]) * 0.5) ||
        (dz > smallestBoxDimension && dz > (boundingBox.coordinates[MAX_Z] - boundingBox.coordinates[MIN_Z]) * 0.5) ) {
        return false;
    }

    // Check the position of the centroid of the bounding box of the patch with reference to the
    // centroid of the cluster
    Vector3D midPatch;

    midPatch.set(
        (patchBoundingBox->coordinates[MIN_X] + patchBoundingBox->coordinates[MAX_X]) / 2.0f,
        (patchBoundingBox->coordinates[MIN_Y] + patchBoundingBox->coordinates[MAX_Y]) / 2.0f,
        (patchBoundingBox->coordinates[MIN_Z] + patchBoundingBox->coordinates[MAX_Z]) / 2.0f);
    // Note: comparator values assumed: X_GREATER, Y_GREATER and Z_GREATER, combined will give
    // an integer number from 0 to 7, or 8 if all are equal
    int selectedChildClusterIndex = vectorCompareByDimensions(&boundingBoxCentroid, &midPatch, EPSILON);

    // If the centroids (almost by EPSILON) coincides, don´t move current patch from parent cluster to sub-cluster
    if ( selectedChildClusterIndex == 0x08 ) {
        return false;
    }

    // Otherwise, move the patch to the sub cluster with index selectedChildClusterIndex
    Cluster *selectedChildCluster = children[selectedChildClusterIndex];

    patches->remove(parentIndex);
    selectedChildCluster->patches->add(patch);

    // Enlarge the bounding box the of sub-cluster
    selectedChildCluster->boundingBox.enlarge(patchBoundingBox);

    // Current patch was moved to the sub-cluster
    return true;
}

/**
Splits a cluster into sub-clusters: it is assumed that cluster is a cluster without
sub-clusters yet. If cluster is nullptr or there are no more than MINIMUM_NUMBER_OF_PATCHES_PER_CLUSTER
patches in the cluster, the routine simply returns. If there are more patches, 8
sub-clusters are created for the cluster. Then for each patch, if the patch
is smaller than an octant, the patch is moved to the octant containing its
centroid. Otherwise, the patch remains a direct child of the cluster. Finally,
sub-clusters with zero patches are disposed off and the procedure recursively repeated
for each sub-cluster
*/
void
Cluster::splitCluster() {
    // Don't split the cluster if it contains too few patches
    if ( patches != nullptr && patches->size() <= MINIMUM_NUMBER_OF_PATCHES_PER_CLUSTER ) {
        return;
    }

    // Create eight sub-clusters for the cluster with initialized bounding box
    for ( int i = 0; i < 8; i++ ) {
        children[i] = new Cluster();
    }

    // Check and possibly move each of the patches in the cluster to a sub-cluster
    for ( int i = 0; patches != nullptr && i < patches->size(); i++ ) {
        if ( clusterMovePatch(i) ) {
            i--;
        }
    }

    // Dispose of sub-clusters containing no patches, call splitCluster recursively for not empty sub-clusters
    for ( int i = 0; i < 8; i++ ) {
        if ( children[i]->patches->size() == 0 ) {
            delete children[i];
            children[i] = nullptr;
        } else {
            children[i]->boundingBoxCentroid.set(
                (children[i]->boundingBox.coordinates[MIN_X] + children[i]->boundingBox.coordinates[MAX_X]) * 0.5f,
                (children[i]->boundingBox.coordinates[MIN_Y] + children[i]->boundingBox.coordinates[MAX_Y]) * 0.5f,
                (children[i]->boundingBox.coordinates[MIN_Z] + children[i]->boundingBox.coordinates[MAX_Z]) * 0.5f);
            if ( children[i] != nullptr ) {
                children[i]->splitCluster();
            }
        }
    }
}

/**
Converts a cluster GLOBAL_stochasticRaytracing_hierarchy to a "normal" Geometry.
The "normal" routines for raytracing can be used to trace a ray through the cluster
GLOBAL_stochasticRaytracing_hierarchy and shaft culling and such can be done on clusters
without extra code and such
*/
Geometry *
Cluster::convertClusterToGeometry() {
    Geometry *parentPatchesGeometry = nullptr;
    if ( patches != nullptr ) {
        parentPatchesGeometry = geomCreatePatchSet(patches);
        addToDeletionCache(parentPatchesGeometry);
    }

    java::ArrayList<Geometry *> *patchesGeometryList = new java::ArrayList<Geometry *>();

    // The patches in the cluster are the first to be tested for intersection with
    if ( parentPatchesGeometry != nullptr ) {
        patchesGeometryList->add(parentPatchesGeometry);
    }

    for ( int i = 0; i < 8; i++ ) {
        Geometry *child = nullptr;
        if ( children[i] != nullptr ) {
            child = children[i]->convertClusterToGeometry();
        }

        if ( child != nullptr ) {
            patchesGeometryList->add(child);
        }
    }

    Compound *newCompound = new Compound(patchesGeometryList);
    Geometry *newGeometry = new Geometry(nullptr, newCompound, GeometryClassId::COMPOUND);
    addToDeletionCache(newGeometry);
    return newGeometry;
}
