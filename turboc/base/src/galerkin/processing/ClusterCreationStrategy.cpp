#include "java/util/ArrayList.txx"
#include "galerkin/processing/ClusterCreationStrategy.h"

ArrayList<GalerkinElement *> *ClusterCreationStrategy::irregularElementsToDelete = NULL;
ArrayList<GalerkinElement *> *ClusterCreationStrategy::hierarchyElements = NULL;

void
ClusterCreationStrategy::addElemTIrrChildrenDltnCch(GalerkinElement *galerkinElement) {
    if ( irregularElementsToDelete == NULL ) {
        irregularElementsToDelete = new ArrayList<GalerkinElement *>();
    }
    irregularElementsToDelete->add(galerkinElement);
}

void
ClusterCreationStrategy::addElemTHrrchDltnCch(GalerkinElement *galerkinElement) {
    if ( hierarchyElements == NULL ) {
        hierarchyElements = new ArrayList<GalerkinElement *>();
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

    if ( galerkinElement->irregularSubElements == NULL ) {
        galerkinElement->irregularSubElements = new ArrayList<Element *>();
        ClusterCreationStrategy::addElemTIrrChildrenDltnCch(galerkinElement);
    }
    galerkinElement->irregularSubElements->add(cluster);
    if ( cluster != NULL ) {
        cluster->parent = galerkinElement;
    }
}

/**
Adds the toplevel (surface) element of the patch to the list of irregular
sub-elements of the galerkinElement
*/
void
ClusterCreationStrategy::patchAddClusterChild(Patch *patch, GalerkinElement *galerkinElement) {
    GalerkinElement *surfaceElement = ((GalerkinElement *)(patch->radianceData));

    if ( galerkinElement->irregularSubElements == NULL ) {
        galerkinElement->irregularSubElements = new ArrayList<Element *>();
        ClusterCreationStrategy::addElemTIrrChildrenDltnCch(galerkinElement);
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
    // Total area of surfaces inside the galerkinElement is sum of the areas of the sub-clusters + pull radiance
    galerkinElement->area = 0.0;
    galerkinElement->numberOfPatches = 0;
    galerkinElement->minimumArea = Numeric::HUGE_FLOAT_VALUE;
    ColorRgb::arrayClear(galerkinElement->radiance, galerkinElement->basisSize);
    for ( int i = 0;
          galerkinElement->irregularSubElements != NULL && i < galerkinElement->irregularSubElements->size();
          i++ ) {
        const GalerkinElement *subCluster = ((GalerkinElement *)(galerkinElement->irregularSubElements->get(i)));
        galerkinElement->area += subCluster->area;
        galerkinElement->numberOfPatches += subCluster->numberOfPatches;
        galerkinElement->radiance[0].addScaled(galerkinElement->radiance[0], subCluster->area, subCluster->radiance[0]);
        if ( subCluster->minimumArea < galerkinElement->minimumArea ) {
            galerkinElement->minimumArea = subCluster->minimumArea;
        }
        galerkinElement->flags |= (subCluster->flags & IS_LIGHT_SOURCE_MASK);
        galerkinElement->Ed.addScaled(galerkinElement->Ed, subCluster->area, subCluster->Ed);
    }
    galerkinElement->radiance[0].scale(1.0f / galerkinElement->area);
    galerkinElement->Ed.scale(1.0f / galerkinElement->area);

    // Also pull un-shot radiance for the "shooting" methods
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        ColorRgb::arrayClear(galerkinElement->unShotRadiance, galerkinElement->basisSize);
        for ( int i = 0;
              galerkinElement->irregularSubElements != NULL && i < galerkinElement->irregularSubElements->size();
              i++ ) {
            const GalerkinElement *subCluster = ((GalerkinElement *)(galerkinElement->irregularSubElements->get(i)));
            galerkinElement->unShotRadiance[0].addScaled(
                galerkinElement->unShotRadiance[0], subCluster->area, subCluster->unShotRadiance[0]);
        }
        galerkinElement->unShotRadiance[0].scale(1.0f / galerkinElement->area);
    }

    // Compute equivalent blocker (or blocker complement) size for multi-resolution visibility
    galerkinElement->blockerSize = galerkinElement->geometry->boundingBox.maxExtent();
}

/**
Creates a cluster for the Geometry, recurse for the children geometries, initializes and
returns the created cluster.

Note that geometry is always of types Compound or PatchSet.
*/
GalerkinElement *
ClusterCreationStrategy::createClusterHierarchy(Geometry *geometry, GalerkinState *galerkinState) {
    if ( geometry == NULL ) {
        return NULL;
    }

    // Parent element
    GalerkinElement *newGalerkinElement = new GalerkinElement(geometry, galerkinState);
    ClusterCreationStrategy::addElemTHrrchDltnCch(newGalerkinElement);

    geometry->radianceData = newGalerkinElement;

    // Recursively creates list of sub-clusters
    if ( geometry->isCompound() ) {
        ArrayList<Geometry *> *geometryList = Geometry::primitiveListCopy(geometry);
        for ( int i = 0; geometryList != NULL && i < geometryList->size(); i++ ) {
            geomAddClusterChild(geometryList->get(i), newGalerkinElement, galerkinState);
        }
        delete geometryList;
    } else {
        const ArrayList<Patch *> *patchList = Geometry::patchListReference(geometry);
        for ( int i = 0; patchList != NULL && i < patchList->size(); i++ ) {
            patchAddClusterChild(patchList->get(i), newGalerkinElement);
        }
    }

    ClusterCreationStrategy::clusterInit(newGalerkinElement, galerkinState);

    return newGalerkinElement;
}

void
ClusterCreationStrategy::freeClusterElements() {
    if ( irregularElementsToDelete != NULL ) {
        for ( int i = 0; i < irregularElementsToDelete->size(); i++ ) {
            GalerkinElement *element = irregularElementsToDelete->get(i);
            delete element->irregularSubElements;
        }

        delete irregularElementsToDelete;
        irregularElementsToDelete = NULL;
    }

    if ( hierarchyElements != NULL ) {
        for ( int i = 0; i < hierarchyElements->size(); i++ ) {
            GalerkinElement *element = hierarchyElements->get(i);
            delete element;
        }
        delete hierarchyElements;
        hierarchyElements = NULL;
    }
}
