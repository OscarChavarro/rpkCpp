#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "common/mymath.h"
#include "GALERKIN/processing/ClusterCreationStrategy.h"

java::ArrayList<GalerkinElement *> *ClusterCreationStrategy::irregularElementsToDelete = nullptr;
java::ArrayList<GalerkinElement *> *ClusterCreationStrategy::hierarchyElements = nullptr;

void
ClusterCreationStrategy::addElementToIrregularChildrenDeletionCache(GalerkinElement *element) {
    if ( irregularElementsToDelete == nullptr ) {
        irregularElementsToDelete = new java::ArrayList<GalerkinElement *>();
    }
    irregularElementsToDelete->add(element);
}

void
ClusterCreationStrategy::addElementToHierarchiesDeletionCache(GalerkinElement *element) {
    if ( hierarchyElements == nullptr ) {
        hierarchyElements = new java::ArrayList<GalerkinElement *>();
    }
    hierarchyElements->add(element);
}

/**
Creates a cluster hierarchy for the Geometry and adds it to the sub-cluster list of the
given parent cluster
*/
void
ClusterCreationStrategy::geomAddClusterChild(Geometry *geometry, GalerkinElement *parentCluster, GalerkinState *galerkinState) {
    GalerkinElement *cluster = ClusterCreationStrategy::galerkinDoCreateClusterHierarchy(geometry, galerkinState);

    if ( parentCluster->irregularSubElements == nullptr ) {
        parentCluster->irregularSubElements = new java::ArrayList<Element *>();
        ClusterCreationStrategy::addElementToIrregularChildrenDeletionCache(parentCluster);
    }
    parentCluster->irregularSubElements->add(cluster);
    if ( cluster != nullptr ) {
        cluster->parent = parentCluster;
    }
}

/**
Adds the toplevel (surface) element of the patch to the list of irregular
sub-elements of the cluster
*/
void
ClusterCreationStrategy::patchAddClusterChild(Patch *patch, GalerkinElement *cluster) {
    GalerkinElement *surfaceElement = (GalerkinElement *)patch->radianceData;

    if ( cluster->irregularSubElements == nullptr ) {
        cluster->irregularSubElements = new java::ArrayList<Element *>();
        ClusterCreationStrategy::addElementToIrregularChildrenDeletionCache(cluster);
    }
    cluster->irregularSubElements->add(surfaceElement);
    surfaceElement->parent = cluster;
}

/**
Initializes the cluster element. Called bottom-up: first the
lowest level clusters and so up
*/
void
ClusterCreationStrategy::clusterInit(GalerkinElement *cluster, const GalerkinState *galerkinState) {
    // Total area of surfaces inside the cluster is sum of the areas of
    // the sub-clusters + pull radiance
    cluster->area = 0.0;
    cluster->numberOfPatches = 0;
    cluster->minimumArea = HUGE_FLOAT_VALUE;
    colorsArrayClear(cluster->radiance, cluster->basisSize);
    for ( int i = 0; cluster->irregularSubElements != nullptr && i< cluster->irregularSubElements->size(); i++ ) {
        const GalerkinElement *subCluster = (GalerkinElement *)cluster->irregularSubElements->get(i);
        cluster->area += subCluster->area;
        cluster->numberOfPatches += subCluster->numberOfPatches;
        cluster->radiance[0].addScaled(cluster->radiance[0], subCluster->area, subCluster->radiance[0]);
        if ( subCluster->minimumArea < cluster->minimumArea ) {
            cluster->minimumArea = subCluster->minimumArea;
        }
        cluster->flags |= (subCluster->flags & IS_LIGHT_SOURCE_MASK);
        cluster->Ed.addScaled(cluster->Ed, subCluster->area, subCluster->Ed);
    }
    cluster->radiance[0].scale(1.0f / cluster->area);
    cluster->Ed.scale(1.0f / cluster->area);

    // Also pull un-shot radiance for the "shooting" methods
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        colorsArrayClear(cluster->unShotRadiance, cluster->basisSize);
        for ( int i = 0; cluster->irregularSubElements != nullptr && i < cluster->irregularSubElements->size(); i++ ) {
            const GalerkinElement *subCluster = (GalerkinElement *)cluster->irregularSubElements->get(i);
            cluster->unShotRadiance[0].addScaled(cluster->unShotRadiance[0], subCluster->area, subCluster->unShotRadiance[0]);
        }
        cluster->unShotRadiance[0].scale(1.0f / cluster->area);
    }

    // Compute equivalent blocker (or blocker complement) size for multi-resolution
    // visibility
    const float *bbx = cluster->geometry->boundingBox.coordinates;
    cluster->blockerSize = java::Math::max((bbx[MAX_X] - bbx[MIN_X]), (bbx[MAX_Y] - bbx[MIN_Y]));
    cluster->blockerSize = java::Math::max(cluster->blockerSize, (bbx[MAX_Z] - bbx[MIN_Z]));
}

/**
Creates a cluster for the Geometry, recursively traverses for the children GEOMs, initializes and
returns the created cluster
*/
GalerkinElement *
ClusterCreationStrategy::galerkinDoCreateClusterHierarchy(Geometry *parentGeometry, GalerkinState *galerkinState) {
    // Geom will be nullptr if e.g. no scene is loaded when selecting
    // Galerkin radiosity for radiance computations
    if ( parentGeometry == nullptr ) {
        return nullptr;
    }

    // Create a cluster for the parentGeometry
    GalerkinElement *cluster = new GalerkinElement(parentGeometry, galerkinState);
    ClusterCreationStrategy::addElementToHierarchiesDeletionCache(cluster);

    parentGeometry->radianceData = cluster;

    // Recursively creates list of sub-clusters
    if ( parentGeometry->isCompound() ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(parentGeometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            geomAddClusterChild(geometryList->get(i), cluster, galerkinState);
        }
        delete geometryList;
    } else {
        const java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(parentGeometry);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
            patchAddClusterChild(patchList->get(i), cluster);
        }
    }

    ClusterCreationStrategy::clusterInit(cluster, galerkinState);

    return cluster;
}

/**
Creates a cluster for the Geometry, recurse for the children geometries, initializes and
returns the created cluster.
*/
GalerkinElement *
ClusterCreationStrategy::createClusterHierarchy(Geometry *geometry, GalerkinState *galerkinState) {
    return ClusterCreationStrategy::galerkinDoCreateClusterHierarchy(geometry, galerkinState);
}

void
ClusterCreationStrategy::freeClusterElements() {
    if ( irregularElementsToDelete != nullptr ) {
        for ( int i = 0; i < irregularElementsToDelete->size(); i++ ) {
            GalerkinElement *element = irregularElementsToDelete->get(i);
            delete element->irregularSubElements;
        }

        delete irregularElementsToDelete;
        irregularElementsToDelete = nullptr;
    }

    if ( hierarchyElements != nullptr ) {
        for ( int i = 0; i < hierarchyElements->size(); i++ ) {
            GalerkinElement *element = hierarchyElements->get(i);
            delete element;
        }
        delete hierarchyElements;
        hierarchyElements = nullptr;
    }
}
