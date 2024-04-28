#include "java/util/ArrayList.txx"
#include "common/mymath.h"
#include "ClusterCreationStrategy.h"

static SGL_CONTEXT *globalSglContext; // Sgl context for determining equivalent blocker sizes
static unsigned char *globalBuffer1; // Needed for eroding and expanding
static unsigned char *globalBuffer2;
static java::ArrayList<GalerkinElement *> *globalIrregularElementsToDelete = nullptr;
static java::ArrayList<GalerkinElement *> *globalHierarchyElements = nullptr;

static GalerkinElement *galerkinDoCreateClusterHierarchy(Geometry *parentGeometry, GalerkinState *galerkinState);

/**
Geoms will be rendered into a frame buffer of this size for determining
the equivalent blocker size
*/
#define FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS 30

/**
Equivalent blocker size determination: first call blockerInit(),
then call GeomBlcokerSize() of GeomBlockserSizeInDirection() for the
geoms for which you like to compute the equivalent blocker size, and
finally terminate with BlockerTerminate()

Creates an sgl context needed for determining the equivalent blocker size
of some objects
*/
static void
blockerInit() {
    globalSglContext = new SGL_CONTEXT(FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS, FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS);
    GLOBAL_sgl_currentContext->sglDepthTesting(true);

    globalBuffer1 = new unsigned char [FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS * FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS];
    globalBuffer2 = new unsigned char [FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS * FRAME_BUFFER_SIDE_LENGTH_IN_PIXELS];
}

/**
Destroys the sgl context created by blockerInit()
*/
static void
blockerTerminate() {
    delete[] globalBuffer2;
    delete[] globalBuffer1;
    delete globalSglContext;
}

static void
addElementToIrregularChildrenDeletionCache(GalerkinElement *element) {
    if ( globalIrregularElementsToDelete == nullptr ) {
        globalIrregularElementsToDelete = new java::ArrayList<GalerkinElement *>();
    }
    globalIrregularElementsToDelete->add(element);
}

static void
addElementToHierarchiesDeletionCache(GalerkinElement *element) {
    if ( globalHierarchyElements == nullptr ) {
        globalHierarchyElements = new java::ArrayList<GalerkinElement *>();
    }
    globalHierarchyElements->add(element);
}

/**
Creates a cluster hierarchy for the Geometry and adds it to the sub-cluster list of the
given parent cluster
*/
static void
geomAddClusterChild(Geometry *geometry, GalerkinElement *parentCluster, GalerkinState *galerkinState) {
    GalerkinElement *cluster = galerkinDoCreateClusterHierarchy(geometry, galerkinState);

    if ( parentCluster->irregularSubElements == nullptr ) {
        parentCluster->irregularSubElements = new java::ArrayList<Element *>();
        addElementToIrregularChildrenDeletionCache(parentCluster);
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
static void
patchAddClusterChild(Patch *patch, GalerkinElement *cluster) {
    GalerkinElement *surfaceElement = (GalerkinElement *)patch->radianceData;

    if ( cluster->irregularSubElements == nullptr ) {
        cluster->irregularSubElements = new java::ArrayList<Element *>();
        addElementToIrregularChildrenDeletionCache(cluster);
    }
    cluster->irregularSubElements->add(surfaceElement);
    surfaceElement->parent = cluster;
}

/**
Initializes the cluster element. Called bottom-up: first the
lowest level clusters and so up
*/
static void
clusterInit(GalerkinElement *cluster, GalerkinState *galerkinState) {
    // Total area of surfaces inside the cluster is sum of the areas of
    // the sub-clusters + pull radiance
    cluster->area = 0.0;
    cluster->numberOfPatches = 0;
    cluster->minimumArea = HUGE;
    clearColorsArray(cluster->radiance, cluster->basisSize);
    for ( int i = 0; cluster->irregularSubElements != nullptr && i< cluster->irregularSubElements->size(); i++ ) {
        GalerkinElement *subCluster = (GalerkinElement *)cluster->irregularSubElements->get(i);
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
        clearColorsArray(cluster->unShotRadiance, cluster->basisSize);
        for ( int i = 0; cluster->irregularSubElements != nullptr && i < cluster->irregularSubElements->size(); i++ ) {
            GalerkinElement *subCluster = (GalerkinElement *)cluster->irregularSubElements->get(i);
            cluster->unShotRadiance[0].addScaled(cluster->unShotRadiance[0], subCluster->area, subCluster->unShotRadiance[0]);
        }
        cluster->unShotRadiance[0].scale(1.0f / cluster->area);
    }

    // Compute equivalent blocker (or blocker complement) size for multi-resolution
    // visibility
    float *bbx = cluster->geometry->boundingBox.coordinates;
    cluster->blockerSize = floatMax((bbx[MAX_X] - bbx[MIN_X]), (bbx[MAX_Y] - bbx[MIN_Y]));
    cluster->blockerSize = floatMax(cluster->blockerSize, (bbx[MAX_Z] - bbx[MIN_Z]));
}

/**
Creates a cluster for the Geometry, recursively traverses for the children GEOMs, initializes and
returns the created cluster
*/
static GalerkinElement *
galerkinDoCreateClusterHierarchy(Geometry *parentGeometry, GalerkinState *galerkinState) {
    // Geom will be nullptr if e.g. no scene is loaded when selecting
    // Galerkin radiosity for radiance computations
    if ( parentGeometry == nullptr ) {
        return nullptr;
    }

    // Create a cluster for the parentGeometry
    GalerkinElement *cluster = new GalerkinElement(parentGeometry, galerkinState);
    addElementToHierarchiesDeletionCache(cluster);

    parentGeometry->radianceData = cluster;

    // Recursively creates list of sub-clusters
    if ( parentGeometry->isCompound() ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(parentGeometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            geomAddClusterChild(geometryList->get(i), cluster, galerkinState);
        }
        delete geometryList;
    } else {
        java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(parentGeometry);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
            patchAddClusterChild(patchList->get(i), cluster);
        }
    }

    clusterInit(cluster, galerkinState);

    return cluster;
}

/**
Creates a cluster for the Geometry, recurse for the children geometries, initializes and
returns the created cluster.

First initializes for equivalent blocker size determination, then calls
the above function, and finally terminates equivalent blocker size
determination
*/
GalerkinElement *
ClusterCreationStrategy::createClusterHierarchy(Geometry *geometry, GalerkinState *galerkinState) {
    blockerInit();
    GalerkinElement *cluster = galerkinDoCreateClusterHierarchy(geometry, galerkinState);
    blockerTerminate();

    return cluster;
}

void
ClusterCreationStrategy::freeClusterElements() {
    if ( globalIrregularElementsToDelete != nullptr ) {
        for ( int i = 0; i < globalIrregularElementsToDelete->size(); i++ ) {
            GalerkinElement *element = globalIrregularElementsToDelete->get(i);
            delete element->irregularSubElements;
        }

        delete globalIrregularElementsToDelete;
        globalIrregularElementsToDelete = nullptr;
    }

    if ( globalHierarchyElements != nullptr ) {
        for ( int i = 0; i < globalHierarchyElements->size(); i++ ) {
            GalerkinElement *element = globalHierarchyElements->get(i);
            delete element;
        }
        delete globalHierarchyElements;
        globalHierarchyElements = nullptr;
    }
}
