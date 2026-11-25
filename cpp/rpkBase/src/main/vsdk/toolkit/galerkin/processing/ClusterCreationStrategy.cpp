#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "java/lang/Float.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/galerkin/processing/ClusterCreationStrategy.h"

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
ClusterCreationStrategy::geomAddClusterChild(RpkGeometry *geometry, GalerkinElement *galerkinElement, GalerkinState *galerkinState) {
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
    GalerkinElement *surfaceElement = galerkinElement->galerkinState->topLevelElement(patch);

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
    // Total area of surfaces inside the galerkinElement is sum of the areas of the sub-clusters + pull radiance
    galerkinElement->area = 0.0;
    galerkinElement->numberOfPatches = 0;
    galerkinElement->minimumArea = java::Float::MAX_VALUE;
    for ( int i = 0; i < galerkinElement->basisSize; i++ ) {
        galerkinElement->radiance[i] = ColorRgb(0.0, 0.0, 0.0);
    }
    double emittance[3] = {galerkinElement->Ed.getR(), galerkinElement->Ed.getG(), galerkinElement->Ed.getB()};
    for ( int i = 0;
          galerkinElement->irregularSubElements != nullptr && i < galerkinElement->irregularSubElements->size();
          i++ ) {
        const GalerkinElement *subCluster = static_cast<GalerkinElement *>(galerkinElement->irregularSubElements->get(i));
        galerkinElement->area += subCluster->area;
        galerkinElement->numberOfPatches += subCluster->numberOfPatches;
        galerkinElement->radiance[0] = galerkinElement->radiance[0].addScaled(subCluster->area, subCluster->radiance[0]);
        if ( subCluster->minimumArea < galerkinElement->minimumArea ) {
            galerkinElement->minimumArea = subCluster->minimumArea;
        }
        galerkinElement->flags |= (subCluster->flags & ElementFlags::IS_LIGHT_SOURCE_MASK);
        emittance[0] += subCluster->area * subCluster->Ed.getR();
        emittance[1] += subCluster->area * subCluster->Ed.getG();
        emittance[2] += subCluster->area * subCluster->Ed.getB();
    }
    galerkinElement->radiance[0] = galerkinElement->radiance[0].multiply(1.0F / galerkinElement->area);
    const double emittanceScale = 1.0F / galerkinElement->area;
    galerkinElement->Ed = ColorRgb(emittance[0] * emittanceScale, emittance[1] * emittanceScale, emittance[2] * emittanceScale);

    // Also pull un-shot radiance for the "shooting" methods
    if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
        for ( int i = 0; i < galerkinElement->basisSize; i++ ) {
            galerkinElement->unShotRadiance[i] = ColorRgb(0.0, 0.0, 0.0);
        }
        for ( int i = 0;
              galerkinElement->irregularSubElements != nullptr && i < galerkinElement->irregularSubElements->size();
              i++ ) {
            const GalerkinElement *subCluster = static_cast<GalerkinElement *>(galerkinElement->irregularSubElements->get(i));
            galerkinElement->unShotRadiance[0] = galerkinElement->unShotRadiance[0].addScaled(subCluster->area, subCluster->unShotRadiance[0]);
        }
        galerkinElement->unShotRadiance[0] = galerkinElement->unShotRadiance[0].multiply(1.0F / galerkinElement->area);
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
ClusterCreationStrategy::createClusterHierarchy(RpkGeometry *geometry, GalerkinState *galerkinState) {
    if ( geometry == nullptr ) {
        return nullptr;
    }

    // Parent element
    GalerkinElement *newGalerkinElement = new GalerkinElement(geometry, galerkinState);
    ClusterCreationStrategy::addElementToHierarchiesDeletionCache(newGalerkinElement);

    geometry->radianceData = newGalerkinElement;

    // Recursively creates list of sub-clusters
    if ( geometry->isCompound() ) {
        java::ArrayList<RpkGeometry *> *geometryList = RpkGeometry::primitiveListCopy(geometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            geomAddClusterChild(geometryList->get(i), newGalerkinElement, galerkinState);
        }
        delete geometryList;
    } else {
        const java::ArrayList<Patch *> *patchList = RpkGeometry::patchListReference(geometry);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
            patchAddClusterChild(patchList->get(i), newGalerkinElement);
        }
    }

    ClusterCreationStrategy::clusterInit(newGalerkinElement, galerkinState);

    return newGalerkinElement;
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
