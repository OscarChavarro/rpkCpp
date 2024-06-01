#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "GALERKIN/processing/ClusterCreationStrategy.h"

java::ArrayList<GalerkinElement *> *ClusterCreationStrategy::irregularElementsToDelete = nullptr;
java::ArrayList<GalerkinElement *> *ClusterCreationStrategy::hierarchyElements = nullptr;

void
ClusterCreationStrategy::addElementToIrregularChildrenDeletionCache(GalerkinElement *galerkinElement) {
    if ( irregularElementsToDelete == nullptr ) {
        irregularElementsToDelete = new java::ArrayList<GalerkinElement *>();
    }
    irregularElementsToDelete->add(galerkinElement);
}

void
ClusterCreationStrategy::addElementToHierarchiesDeletionCache(GalerkinElement *galerkinElement) {
    if ( hierarchyElements == nullptr ) {
        hierarchyElements = new java::ArrayList<GalerkinElement *>();
    }
    hierarchyElements->add(galerkinElement);
}

/**
Creates a cluster hierarchy for the Geometry and adds it to the sub-cluster list of the
given parent cluster
*/
void
ClusterCreationStrategy::geomAddClusterChild(Geometry *geometry, GalerkinElement *galerkinElement, GalerkinState *galerkinState) {
    GalerkinElement *cluster = ClusterCreationStrategy::createClusterHierarchy(geometry, galerkinState);

    if ( galerkinElement->irregularSubElements == nullptr ) {
        galerkinElement->irregularSubElements = new java::ArrayList<Element *>();
        ClusterCreationStrategy::addElementToIrregularChildrenDeletionCache(galerkinElement);
    }
    galerkinElement->irregularSubElements->add(cluster);
    if ( cluster != nullptr ) {
        cluster->parent = galerkinElement;
    }
}

/**
Adds the toplevel (surface) element of the patch to the list of irregular
sub-elements of the galerkinElement
*/
void
ClusterCreationStrategy::patchAddClusterChild(Patch *patch, GalerkinElement *galerkinElement) {
    GalerkinElement *surfaceElement = (GalerkinElement *)patch->radianceData;

    if ( galerkinElement->irregularSubElements == nullptr ) {
        galerkinElement->irregularSubElements = new java::ArrayList<Element *>();
        ClusterCreationStrategy::addElementToIrregularChildrenDeletionCache(galerkinElement);
    }
    galerkinElement->irregularSubElements->add(surfaceElement);
    surfaceElement->parent = galerkinElement;
}

/**
Initializes the galerkinElement element. Called bottom-up: first the
lowest level clusters and so up
*/
void
ClusterCreationStrategy::clusterInit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) {
    // Total area of surfaces inside the galerkinElement is sum of the areas of
    // the sub-clusters + pull radiance
    galerkinElement->area = 0.0;
    galerkinElement->numberOfPatches = 0;
    galerkinElement->minimumArea = Numeric::HUGE_FLOAT_VALUE;
    colorsArrayClear(galerkinElement->radiance, galerkinElement->basisSize);
    for ( int i = 0; galerkinElement->irregularSubElements != nullptr && i < galerkinElement->irregularSubElements->size(); i++ ) {
        const GalerkinElement *subCluster = (GalerkinElement *)galerkinElement->irregularSubElements->get(i);
        galerkinElement->area += subCluster->area;
        galerkinElement->numberOfPatches += subCluster->numberOfPatches;
        galerkinElement->radiance[0].addScaled(galerkinElement->radiance[0], subCluster->area, subCluster->radiance[0]);
        if ( subCluster->minimumArea < galerkinElement->minimumArea ) {
            galerkinElement->minimumArea = subCluster->minimumArea;
        }
        galerkinElement->flags |= (subCluster->flags & ElementFlags::IS_LIGHT_SOURCE_MASK);
        galerkinElement->Ed.addScaled(galerkinElement->Ed, subCluster->area, subCluster->Ed);
    }
    galerkinElement->radiance[0].scale(1.0f / galerkinElement->area);
    galerkinElement->Ed.scale(1.0f / galerkinElement->area);

    // Also pull un-shot radiance for the "shooting" methods
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        colorsArrayClear(galerkinElement->unShotRadiance, galerkinElement->basisSize);
        for ( int i = 0; galerkinElement->irregularSubElements != nullptr && i < galerkinElement->irregularSubElements->size(); i++ ) {
            const GalerkinElement *subCluster = (GalerkinElement *)galerkinElement->irregularSubElements->get(i);
            galerkinElement->unShotRadiance[0].addScaled(galerkinElement->unShotRadiance[0], subCluster->area, subCluster->unShotRadiance[0]);
        }
        galerkinElement->unShotRadiance[0].scale(1.0f / galerkinElement->area);
    }

    // Compute equivalent blocker (or blocker complement) size for multi-resolution
    // visibility
    const float *bbx = galerkinElement->geometry->boundingBox.coordinates;
    galerkinElement->blockerSize = java::Math::max((bbx[MAX_X] - bbx[MIN_X]), (bbx[MAX_Y] - bbx[MIN_Y]));
    galerkinElement->blockerSize = java::Math::max(galerkinElement->blockerSize, (bbx[MAX_Z] - bbx[MIN_Z]));
}

/**
Creates a cluster for the Geometry, recurse for the children geometries, initializes and
returns the created cluster.
*/
GalerkinElement *
ClusterCreationStrategy::createClusterHierarchy(Geometry *geometry, GalerkinState *galerkinState) {
    if ( geometry == nullptr ) {
        return nullptr;
    }

    // Parent element
    GalerkinElement *cluster = new GalerkinElement(geometry, galerkinState);
    ClusterCreationStrategy::addElementToHierarchiesDeletionCache(cluster);

    geometry->radianceData = cluster;

    // Recursively creates list of sub-clusters
    if ( geometry->isCompound() ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            geomAddClusterChild(geometryList->get(i), cluster, galerkinState);
        }
        delete geometryList;
    } else {
        const java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(geometry);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
            patchAddClusterChild(patchList->get(i), cluster);
        }
    }

    ClusterCreationStrategy::clusterInit(cluster, galerkinState);

    return cluster;
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

void
ClusterCreationStrategy::printGalerkinElementHierarchy(const GalerkinElement *galerkinElement, const int level) {
    if ( level == 0 ) {
        printf("= GalerkinElement hierarchy ================================================================\n");
    }
    switch ( level ) {
        case 0:
            break;
        case 1:
            printf("* ");
            break;
        case 2:
            printf("  - ");
            break;
        case 3:
            printf("    . ");
            break;
        default:
            printf("   ");
            for ( int j = 0; j < level; j++ ) {
                printf(" ");
            }
            printf("[%d] ", level);
            break;
    }
    if ( galerkinElement != nullptr ) {
        printf("%d ( ", galerkinElement->id);
        if ( galerkinElement->geometry != nullptr ) {
            switch ( galerkinElement->geometry->className ) {
                case GeometryClassId::PATCH_SET:
                    printf("geom patchSet");
                    break;
                case GeometryClassId::SURFACE_MESH:
                    printf("geom mesh");
                    break;
                case GeometryClassId::COMPOUND:
                    printf("geom compound");
                    break;
                default:
                    break;
            }
            printf(" %d ", galerkinElement->geometry->id);
        }
        if ( galerkinElement->patch != nullptr ) {
            printf("patch %d ", galerkinElement->patch->id);
        }
        printf(")\n");
        for ( int i = 0;
              galerkinElement->irregularSubElements != nullptr && i < galerkinElement->irregularSubElements->size();
              i++ ) {
            printGalerkinElementHierarchy((const GalerkinElement *) galerkinElement->irregularSubElements->get(i),
                                          level + 1);
        }
    } else {
        printf("NULL\n");
    }
}