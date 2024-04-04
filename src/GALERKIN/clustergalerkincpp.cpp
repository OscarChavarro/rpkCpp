/**
Cluster-specific operations
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "skin/Geometry.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/scratch.h"
#include "GALERKIN/mrvisibility.h"

static ColorRgb globalSourceRadiance;
static Vector3D globalSamplePoint;
static double globalProjectedArea;
static ColorRgb *globalPSourceRad;
static Interaction *globalTheLink;

// Area corresponding to one pixel in the scratch frame buffer
static double globalPixelArea;

static GalerkinElement *galerkinDoCreateClusterHierarchy(Geometry *parentGeometry);

/**
Creates a cluster hierarchy for the Geometry and adds it to the sub-cluster list of the
given parent cluster
*/
static void
geomAddClusterChild(Geometry *geom, GalerkinElement *parent_cluster) {
    GalerkinElement *cluster = galerkinDoCreateClusterHierarchy(geom);

    if ( parent_cluster->irregularSubElements == nullptr ) {
        parent_cluster->irregularSubElements = new java::ArrayList<Element *>();
    }
    parent_cluster->irregularSubElements->add(0, cluster);
    cluster->parent = parent_cluster;
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
    }
    cluster->irregularSubElements->add(0, surfaceElement);
    surfaceElement->parent = cluster;
}

/**
Initializes the cluster element. Called bottom-up: first the
lowest level clusters and so up
*/
static void
clusterInit(GalerkinElement *cluster) {
    // Total area of surfaces inside the cluster is sum of the areas of
    // the sub-clusters + pull radiance
    cluster->area = 0.0;
    cluster->numberOfPatches = 0;
    cluster->minimumArea = HUGE;
    clusterGalerkinClearCoefficients(cluster->radiance, cluster->basisSize);
    for ( int i = 0; cluster->irregularSubElements != nullptr && i< cluster->irregularSubElements->size(); i++ ) {
        GalerkinElement *subCluster = (GalerkinElement *)cluster->irregularSubElements->get(i);
        cluster->area += subCluster->area;
        cluster->numberOfPatches += subCluster->numberOfPatches;
        colorAddScaled(cluster->radiance[0], subCluster->area, subCluster->radiance[0], cluster->radiance[0]);
        if ( subCluster->minimumArea < cluster->minimumArea ) {
            cluster->minimumArea = subCluster->minimumArea;
        }
        cluster->flags |= (subCluster->flags & IS_LIGHT_SOURCE_MASK);
        colorAddScaled(cluster->Ed, subCluster->area, subCluster->Ed, cluster->Ed);
    }
    colorScale((1.0f / cluster->area), cluster->radiance[0], cluster->radiance[0]);
    colorScale((1.0f / cluster->area), cluster->Ed, cluster->Ed);

    // Also pull un-shot radiance for the "shooting" methods
    if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL ) {
        clusterGalerkinClearCoefficients(cluster->unShotRadiance, cluster->basisSize);
        for ( int i = 0; cluster->irregularSubElements != nullptr && i < cluster->irregularSubElements->size(); i++ ) {
            GalerkinElement *subCluster = (GalerkinElement *)cluster->irregularSubElements->get(i);
            colorAddScaled(cluster->unShotRadiance[0], subCluster->area, subCluster->unShotRadiance[0],
                           cluster->unShotRadiance[0]);
        }
        colorScale((1.0f / cluster->area), cluster->unShotRadiance[0], cluster->unShotRadiance[0]);
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
galerkinDoCreateClusterHierarchy(Geometry *parentGeometry) {
    // Geom will be nullptr if e.g. no scene is loaded when selecting
    // Galerkin radiosity for radiance computations
    if ( parentGeometry == nullptr ) {
        return nullptr;
    }

    // Create a cluster for the parentGeometry
    GalerkinElement *cluster = new GalerkinElement(parentGeometry);
    parentGeometry->radianceData = cluster;

    // Recursively creates list of sub-clusters
    if ( parentGeometry->isCompound() ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(parentGeometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            geomAddClusterChild(geometryList->get(i), cluster);
        }
        delete geometryList;
    } else {
        java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(parentGeometry);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
            patchAddClusterChild(patchList->get(i), cluster);
        }
    }

    clusterInit(cluster);

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
galerkinCreateClusterHierarchy(Geometry *geom) {
    blockerInit();
    GalerkinElement *cluster = galerkinDoCreateClusterHierarchy(geom);
    blockerTerminate();

    return cluster;
}

/**
Disposes of the cluster hierarchy
*/
void
galerkinDestroyClusterHierarchy(GalerkinElement *clusterElement) {
    if ( !clusterElement || !clusterElement->isCluster() ) {
        return;
    }

    for ( int i = 0; clusterElement->irregularSubElements != nullptr && i < clusterElement->irregularSubElements->size(); i++ ) {
        galerkinDestroyClusterHierarchy((GalerkinElement *)clusterElement->irregularSubElements->get(i));
    }
    delete clusterElement;
}

/**
Executes func for every surface element in the cluster
*/
void
iterateOverSurfaceElementsInCluster(GalerkinElement *galerkinElement, void (*func)(GalerkinElement *elem) ) {
    if ( !galerkinElement->isCluster() ) {
        func(galerkinElement);
    } else {
        for ( int i = 0; galerkinElement->irregularSubElements != nullptr && i < galerkinElement->irregularSubElements->size(); i++ ) {
            GalerkinElement *subCluster = (GalerkinElement *)galerkinElement->irregularSubElements->get(i);
            iterateOverSurfaceElementsInCluster(subCluster, func);
        }
    }
}

/**
Uses global variables globalSourceRadiance and globalSamplePoint: accumulates the
power emitted by the element towards the globalSamplePoint in globalSourceRadiance
only taking into account the surface orientation w.r.t. the
sample point, (ignores intra cluster visibility)
*/
static void
accumulatePowerToSamplePoint(GalerkinElement *src) {
    float srcOs;
    float dist;
    Vector3D dir;
    ColorRgb rad;

    vectorSubtract(globalSamplePoint, src->patch->midPoint, dir);
    dist = vectorNorm(dir);
    if ( dist < EPSILON ) {
        srcOs = 1.0f;
    } else {
        srcOs = vectorDotProduct(dir, src->patch->normal) / dist;
    }
    if ( srcOs <= 0.0f ) {
        // Receiver point is behind the src
        return;
    }

    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
         GLOBAL_galerkin_state.iteration_method == JACOBI ) {
        rad = src->radiance[0];
    } else {
        rad = src->unShotRadiance[0];
    }

    colorAddScaled(globalSourceRadiance, srcOs * src->area, rad, globalSourceRadiance);
}

/**
Returns the radiance or un-shot radiance (depending on the
iteration method) emitted by the source element, a cluster,
towards the sample point
*/
ColorRgb
clusterRadianceToSamplePoint(GalerkinElement *src, Vector3D sample) {
    switch ( GLOBAL_galerkin_state.clusteringStrategy ) {
        case ISOTROPIC:
            return src->radiance[0];

        case ORIENTED: {
            globalSamplePoint = sample;

            // Accumulate the power emitted by the patches in the source cluster
            // towards the sample point
            globalSourceRadiance.clear();
            iterateOverSurfaceElementsInCluster(src, accumulatePowerToSamplePoint);

            // Divide by the source area used for computing the form factor:
            // src->area / 4.0 (average projected area)
            colorScale(4.0f / src->area, globalSourceRadiance, globalSourceRadiance);
            return globalSourceRadiance;
        }

        case Z_VISIBILITY:
            if ( !src->isCluster() || !src->geometry->boundingBox.outOfBounds(&sample) ) {
                return src->radiance[0];
            } else {
                double areaFactor;

                // Render pointers to the elements in the source cluster into the scratch frame
                // buffer, seen from the sample point
                float *bbx = scratchRenderElements(src, sample);

                // Compute average radiance on the virtual screen
                globalSourceRadiance = scratchRadiance();

                // Area factor = area of virtual screen / source cluster area used for
                // form factor computation
                areaFactor = ((bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y])) /
                             (0.25 * src->area);
                colorScale((float)areaFactor, globalSourceRadiance, globalSourceRadiance);
                return globalSourceRadiance;
            }

        default:
            logFatal(-1, "clusterRadianceToSamplePoint", "Invalid clustering strategy %d\n",
                     GLOBAL_galerkin_state.clusteringStrategy);
    }

    return globalSourceRadiance; // This point is never reached
}

/**
Determines the average radiance or un-shot radiance (depending on
the iteration method) emitted by the source cluster towards the
receiver in the link. The source should be a cluster
*/
ColorRgb
sourceClusterRadiance(Interaction *link) {
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;

    if ( !src->isCluster() || src == rcv ) {
        logFatal(-1, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    // Take a sample point on the receiver
    return clusterRadianceToSamplePoint(src, rcv->midPoint());
}

/**
Computes projected area of receiver surface element towards the sample point
(global variable). Ignores intra cluster visibility
*/
static double
surfaceProjectedAreaToSamplePoint(GalerkinElement *rcv) {
    double rcvCos;
    double dist;
    Vector3D dir;

    vectorSubtract(globalSamplePoint, rcv->patch->midPoint, dir);
    dist = vectorNorm(dir);
    if ( dist < EPSILON ) {
        rcvCos = 1.0;
    } else {
        rcvCos = vectorDotProduct(dir, rcv->patch->normal) / dist;
    }
    if ( rcvCos <= 0.0 ) {
        // Sample point is behind the rcv
        return 0.0;
    }

    return rcvCos * rcv->area;
}

static void
accumulateProjectedAreaToSamplePoint(GalerkinElement *rcv) {
    globalProjectedArea += surfaceProjectedAreaToSamplePoint(rcv);
}

/**
Computes projected area of receiver cluster as seen from the midpoint of the source,
ignoring intra-receiver visibility
*/
double
receiverClusterArea(Interaction *link) {
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;

    if ( !rcv->isCluster() || src == rcv ) {
        return rcv->area;
    }

    switch ( GLOBAL_galerkin_state.clusteringStrategy ) {
        case ISOTROPIC:
            return rcv->area;

        case ORIENTED: {
            globalSamplePoint = src->midPoint();
            globalProjectedArea = 0.0;
            iterateOverSurfaceElementsInCluster(rcv, accumulateProjectedAreaToSamplePoint);
            return globalProjectedArea;
        }

        case Z_VISIBILITY:
            globalSamplePoint = src->midPoint();
            if ( !rcv->geometry->boundingBox.outOfBounds(&globalSamplePoint) ) {
                return rcv->area;
            } else {
                float *bbx = scratchRenderElements(rcv, globalSamplePoint);

                // Projected area is the number of non-background pixels over
                // the total number of pixels * area of the virtual screen
                globalProjectedArea = (double) scratchNonBackgroundPixels() *
                                      (bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) /
                                      (double) (GLOBAL_galerkin_state.scratch->vp_width * GLOBAL_galerkin_state.scratch->vp_height);
                return globalProjectedArea;
            }

        default:
            logFatal(-1, "receiverClusterArea", "Invalid clustering strategy %d",
                     GLOBAL_galerkin_state.clusteringStrategy);
    }

    return 0.0; // This point is never reached and if it is it's your fault
}

/**
Gathers radiance over the interaction, from the interaction source to
the specified receiver element. area_factor is a correction factor
to account for the fact that the receiver cluster area/4 was used in
form factor computations instead of the true receiver area. The source
radiance is explicit given
*/
static void
doGatherRadiance(GalerkinElement *rcv, double area_factor, Interaction *link, ColorRgb *srcRad) {
    ColorRgb *rcvRad = rcv->receivedRadiance;

    if ( link->numberOfBasisFunctionsOnReceiver == 1 && link->numberOfBasisFunctionsOnSource == 1 ) {
        colorAddScaled(rcvRad[0], (float)(area_factor * link->K[0]), srcRad[0], rcvRad[0]);
    } else {
        int alpha;
        int beta;
        int a;
        int b;
        a = intMin(link->numberOfBasisFunctionsOnReceiver, rcv->basisSize);
        b = intMin(link->numberOfBasisFunctionsOnSource, link->sourceElement->basisSize);
        for ( alpha = 0; alpha < a; alpha++ ) {
            for ( beta = 0; beta < b; beta++ ) {
                colorAddScaled(rcvRad[alpha],
                               (float)(area_factor * link->K[alpha * link->numberOfBasisFunctionsOnSource + beta]),
                               srcRad[beta], rcvRad[alpha]);
            }
        }
    }
}

/**
Requires global variables globalPSourceRad (globalSourceRadiance pointer), globalTheLink (interaction
over which is being gathered) and globalSamplePoint (midpoint of source
element). rcv is a surface element belonging to the receiver cluster
in the interaction. This routines gathers radiance to this receiver
surface, taking into account the projected area of the receiver
towards the midpoint of the source, ignoring visibility in the receiver
cluster
*/
static void
orientedSurfaceGatherRadiance(GalerkinElement *rcv) {
    double area_factor;

    // globalTheLink->rcv is a cluster, so it's total area divided by 4 (average projected area)
    // was used to compute link->K
    area_factor = surfaceProjectedAreaToSamplePoint(rcv) / (0.25 * globalTheLink->receiverElement->area);

    doGatherRadiance(rcv, area_factor, globalTheLink, globalPSourceRad);
}

/**
Same as above, except that the number of pixels in the scratch frame buffer
times the area corresponding one such pixel is used as the visible area
of the element. Uses global variables globalPixelArea, globalTheLink, globalPSourceRad
*/
static void
ZVisSurfaceGatherRadiance(GalerkinElement *rcv) {
    double area_factor;

    if ( rcv->tmp <= 0 ) {
        // Element occupies no pixels in the scratch frame buffer
        return;
    }

    area_factor = globalPixelArea * (double) (rcv->tmp) / (0.25 * globalTheLink->receiverElement->area);
    doGatherRadiance(rcv, area_factor, globalTheLink, globalPSourceRad);

    rcv->tmp = 0; // Set it to zero for future re-use
}

/**
Distributes the source radiance to the surface elements in the
receiver cluster
*/
void
clusterGatherRadiance(Interaction *link, ColorRgb *srcRad) {
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;

    if ( !rcv->isCluster() || src == rcv ) {
        logFatal(-1, "clusterGatherRadiance", "Source and receiver are the same or receiver is not a cluster");
        return;
    }

    globalPSourceRad = srcRad;
    globalTheLink = link;
    globalSamplePoint = src->midPoint();

    switch ( GLOBAL_galerkin_state.clusteringStrategy ) {
        case ISOTROPIC:
            doGatherRadiance(rcv, 1.0, link, srcRad);
            break;
        case ORIENTED:
            iterateOverSurfaceElementsInCluster(rcv, orientedSurfaceGatherRadiance);
            break;
        case Z_VISIBILITY:
            if ( !rcv->geometry->boundingBox.outOfBounds(&globalSamplePoint) ) {
                iterateOverSurfaceElementsInCluster(rcv, orientedSurfaceGatherRadiance);
            } else {
                float *bbx = scratchRenderElements(rcv, globalSamplePoint);

                // Count how many pixels each element occupies in the scratch frame buffer
                scratchPixelsPerElement();

                // Area corresponding to one pixel on the virtual screen
                globalPixelArea = (bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) /
                                  (double) (GLOBAL_galerkin_state.scratch->vp_width * GLOBAL_galerkin_state.scratch->vp_height);

                // Gathers the radiance to each element that occupies at least one
                // pixel in the scratch frame buffer and sets elem->tmp back to zero
                // for those elements
                iterateOverSurfaceElementsInCluster(rcv, ZVisSurfaceGatherRadiance);
            }
            break;
        default:
            logFatal(-1, "clusterGatherRadiance", "Invalid clustering strategy %d",
                     GLOBAL_galerkin_state.clusteringStrategy);
    }
}

static void
determineMaxRadiance(GalerkinElement *elem) {
    ColorRgb rad;
    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
         GLOBAL_galerkin_state.iteration_method == JACOBI ) {
        rad = elem->radiance[0];
    } else {
        rad = elem->unShotRadiance[0];
    }
    colorMaximum(globalSourceRadiance, rad, globalSourceRadiance);
}

ColorRgb
maxClusterRadiance(GalerkinElement *cluster) {
    globalSourceRadiance.clear();
    iterateOverSurfaceElementsInCluster(cluster, determineMaxRadiance);
    return globalSourceRadiance;
}
