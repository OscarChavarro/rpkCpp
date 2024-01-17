/**
Cluster-specific operations
*/

#include "common/error.h"
#include "skin/Geometry.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/scratch.h"
#include "GALERKIN/mrvisibility.h"

static COLOR globalSourceRadiance;
static Vector3D globalSamplePoint;
static double globalProjectedArea;
static COLOR *globalPsrcRad;
static INTERACTION *globalTheLink;

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

    parent_cluster->irregular_subelements = ElementListAdd(parent_cluster->irregular_subelements, cluster);
    cluster->parent = parent_cluster;
}

/**
Adds the toplevel (surface) element of the patch to the list of irregular
sub-elements of the cluster
*/
static void
patchAddClusterChild(Patch *patch, GalerkinElement *cluster) {
    GalerkinElement *surfaceElement = (GalerkinElement *)patch->radiance_data;

    cluster->irregular_subelements = ElementListAdd(cluster->irregular_subelements, surfaceElement);
    surfaceElement->parent = cluster;
}

/**
Initializes the cluster element. Called bottom-up: first the
lowest level clusters and so up
*/
static void
clusterInit(GalerkinElement *cluster) {
    ELEMENTLIST *subellist;

    /* total area of surfaces inside the cluster is sum of the areas of
     * the subclusters + pull radiance. */
    cluster->area = 0.;
    cluster->nrpatches = 0;
    cluster->minarea = HUGE;
    clusterGalerkinClearCoefficients(cluster->radiance, cluster->basis_size);
    for ( subellist = cluster->irregular_subelements; subellist; subellist = subellist->next ) {
        GalerkinElement *subclus = subellist->element;
        cluster->area += subclus->area;
        cluster->nrpatches += subclus->nrpatches;
        colorAddScaled(cluster->radiance[0], subclus->area, subclus->radiance[0], cluster->radiance[0]);
        if ( subclus->minarea < cluster->minarea ) {
            cluster->minarea = subclus->minarea;
        }
        cluster->flags |= (subclus->flags & IS_LIGHT_SOURCE);
        colorAddScaled(cluster->Ed, subclus->area, subclus->Ed, cluster->Ed);
    }
    colorScale((1.0f / cluster->area), cluster->radiance[0], cluster->radiance[0]);
    colorScale((1.0f / cluster->area), cluster->Ed, cluster->Ed);

    /* also pull unshot radiance for the "shooting" methods */
    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        clusterGalerkinClearCoefficients(cluster->unshot_radiance, cluster->basis_size);
        for ( subellist = cluster->irregular_subelements; subellist; subellist = subellist->next ) {
            GalerkinElement *subclus = subellist->element;
            colorAddScaled(cluster->unshot_radiance[0], subclus->area, subclus->unshot_radiance[0],
                           cluster->unshot_radiance[0]);
        }
        colorScale((1.0f / cluster->area), cluster->unshot_radiance[0], cluster->unshot_radiance[0]);
    }

    /* compute equivalent blocker (or blocker complement) size for multi-resolution
     * visibility */
    /* if (GLOBAL_galerkin_state.multi-res_visibility) */ {
        /* This is very costly
        cluster->bsize = GeomBlockerSize(cluster->pog.geom);
        */
        float *bbx = cluster->geom->bounds;
        cluster->bsize = MAX((bbx[MAX_X] - bbx[MIN_X]), (bbx[MAX_Y] - bbx[MIN_Y]));
        cluster->bsize = MAX(cluster->bsize, (bbx[MAX_Z] - bbx[MIN_Z]));
    } /* else
    cluster->bsize = 2.*sqrt((cluster->area/4.)/M_PI); */
}

/**
Creates a cluster for the Geometry, recursively traverses for the children GEOMs, initializes and
returns the created cluster
*/
static GalerkinElement *
galerkinDoCreateClusterHierarchy(Geometry *parentGeometry) {
    // Geom will be nullptr if e.g. no scene is loaded when selecting
    // Galerkin radiosity for radiance computations
    if ( !parentGeometry ) {
        return nullptr;
    }

    /* create a cluster for the parentGeometry */
    GalerkinElement *cluster = galerkinElementCreateCluster(parentGeometry);
    parentGeometry->radiance_data = (void *) cluster;

    // Recursively creates list of sub-clusters
    if ( geomIsAggregate(parentGeometry) ) {
        for ( GeometryListNode *window = geomPrimList(parentGeometry); window != nullptr; window = window->next ) {
            Geometry *childGeometry = window->geom;
            geomAddClusterChild(childGeometry, cluster);
        }
    } else {
        for ( PatchSet *window = geomPatchList(parentGeometry); window != nullptr; window = window->next ) {
            patchAddClusterChild(window->patch, cluster);
        }
    }

    clusterInit(cluster);

    return cluster;
}

/**
Creates a cluster for the Geometry, recurses for the children geometries, initializes and
returns the created cluster.

First initializes for equivalent blocker size determination, then calls
the above function, and finally terminates equivalent blocker size
determination
*/
GalerkinElement *
galerkinCreateClusterHierarchy(Geometry *geom) {
    BlockerInit();
    GalerkinElement *cluster = galerkinDoCreateClusterHierarchy(geom);
    BlockerTerminate();

    return cluster;
}

/**
Disposes of the cluster hierarchy
*/
void
galerkinDestroyClusterHierarchy(GalerkinElement *cluster) {
    if ( !cluster || !isCluster(cluster)) {
        return;
    }

    ITERATE_IRREGULAR_SUBELEMENTS(cluster, galerkinDestroyClusterHierarchy);
    galerkinElementDestroyCluster(cluster);
}

/**
Executes func for every surface element in the cluster
*/
void
iterateOverSurfaceElementsInCluster(GalerkinElement *clus, void (*func)(GalerkinElement *elem)) {
    if ( !isCluster(clus)) {
        func(clus);
    } else {
        ELEMENTLIST *subcluslist;
        for ( subcluslist = clus->irregular_subelements; subcluslist; subcluslist = subcluslist->next ) {
            GalerkinElement *subclus = subcluslist->element;
            iterateOverSurfaceElementsInCluster(subclus, func);
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
    float srcos;
    float dist;
    Vector3D dir;
    COLOR rad;

    VECTORSUBTRACT(globalSamplePoint, src->patch->midpoint, dir);
    dist = VECTORNORM(dir);
    if ( dist < EPSILON ) {
        srcos = 1.0f;
    } else {
        srcos = VECTORDOTPRODUCT(dir, src->patch->normal) / dist;
    }
    if ( srcos <= 0.0f ) {
        return;
    }    /* receiver point is behind the src */

    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
         GLOBAL_galerkin_state.iteration_method == JACOBI ) {
        rad = src->radiance[0];
    } else {
        rad = src->unshot_radiance[0];
    }

    colorAddScaled(globalSourceRadiance, srcos * src->area, rad, globalSourceRadiance);
}

/**
Returns the radiance or un-shot radiance (depending on the
iteration method) emitted by the source element, a cluster,
towards the sample point
*/
COLOR
clusterRadianceToSamplePoint(GalerkinElement *src, Vector3D sample) {
    switch ( GLOBAL_galerkin_state.clustering_strategy ) {
        case ISOTROPIC:
            return src->radiance[0];

        case ORIENTED: {
            globalSamplePoint = sample;

            /* accumulate the power emitted by the patches in the source cluster
             * towards the sample point. */
            colorClear(globalSourceRadiance);
            iterateOverSurfaceElementsInCluster(src, accumulatePowerToSamplePoint);

            /* divide by the source area used for computing the form factor:
             * src->area / 4. (average projected area) */
            colorScale(4. / src->area, globalSourceRadiance, globalSourceRadiance);
            return globalSourceRadiance;
        }

        case Z_VISIBILITY:
            if ( !isCluster(src) || !OutOfBounds(&sample, src->geom->bounds)) {
                return src->radiance[0];
            } else {
                double areafactor;

                /* Render pointers to the elements in the source cluster into the scratch frame
                 * buffer as seen from the sample point. */
                float *bbx = ScratchRenderElementPtrs(src, sample);

                /* Compute average radiance on the virtual screen */
                globalSourceRadiance = ScratchRadiance();

                /* areafactor = area of virtual screen / source cluster area used for
                 * form factor computation */
                areafactor = ((bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y])) /
                             (0.25 * src->area);
                colorScale(areafactor, globalSourceRadiance, globalSourceRadiance);
                return globalSourceRadiance;
            }

        default:
            logFatal(-1, "clusterRadianceToSamplePoint", "Invalid clustering strategy %d\n",
                     GLOBAL_galerkin_state.clustering_strategy);
    }

    return globalSourceRadiance;    /* this point is never reached */
}

/**
Determines the average radiance or un-shot radiance (depending on
the iteration method) emitted by the source cluster towards the
receiver in the link. The source should be a cluster
*/
COLOR
sourceClusterRadiance(INTERACTION *link) {
    GalerkinElement *src = link->src, *rcv = link->rcv;

    if ( !isCluster(src) || src == rcv ) {
        logFatal(-1, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    /* take a sample point on the receiver */
    return clusterRadianceToSamplePoint(src, galerkinElementMidPoint(rcv));
}

/**
Computes projected area of receiver surface element towards the sample point
(global variable). Ignores intra cluster visibility
*/
static double
surfaceProjectedAreaToSamplePoint(GalerkinElement *rcv) {
    double rcvcos, dist;
    Vector3D dir;

    VECTORSUBTRACT(globalSamplePoint, rcv->patch->midpoint, dir);
    dist = VECTORNORM(dir);
    if ( dist < EPSILON ) {
        rcvcos = 1.;
    } else {
        rcvcos = VECTORDOTPRODUCT(dir, rcv->patch->normal) / dist;
    }
    if ( rcvcos <= 0. ) {
        return 0.;
    }    /* sample point is behind the rcv */

    return rcvcos * rcv->area;
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
receiverClusterArea(INTERACTION *link) {
    GalerkinElement *src = link->src, *rcv = link->rcv;

    if ( !isCluster(rcv) || src == rcv ) {
        return rcv->area;
    }

    switch ( GLOBAL_galerkin_state.clustering_strategy ) {
        case ISOTROPIC:
            return rcv->area;

        case ORIENTED: {
            globalSamplePoint = galerkinElementMidPoint(src);
            globalProjectedArea = 0.;
            iterateOverSurfaceElementsInCluster(rcv, accumulateProjectedAreaToSamplePoint);
            return globalProjectedArea;
        }

        case Z_VISIBILITY:
            globalSamplePoint = galerkinElementMidPoint(src);
            if ( !OutOfBounds(&globalSamplePoint, rcv->geom->bounds)) {
                return rcv->area;
            } else {
                float *bbx = ScratchRenderElementPtrs(rcv, globalSamplePoint);

                /* projected area is the number of non background pixels over
                 * the total number of pixels * area of the virtual screen. */
                globalProjectedArea = (double) ScratchNonBackgroundPixels() *
                                      (bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) /
                                      (double) (GLOBAL_galerkin_state.scratch->vp_width * GLOBAL_galerkin_state.scratch->vp_height);
                return globalProjectedArea;
            }

        default:
            logFatal(-1, "receiverClusterArea", "Invalid clustering strategy %d",
                     GLOBAL_galerkin_state.clustering_strategy);
    }

    return 0.;    /* this point is never reached and if it is it's your fault. */
}

/**
Gathers radiance over the interaction, from the interaction source to
the specified receiver element. area_factor is a correction factor
to account for the fact that the receiver cluster area/4 was used in
form factor computations instead of the true receiver area. The source
radiance is explicitely given
*/
static void
doGatherRadiance(GalerkinElement *rcv, double area_factor, INTERACTION *link, COLOR *srcrad) {
    COLOR *rcvrad = rcv->received_radiance;

    if ( link->nrcv == 1 && link->nsrc == 1 ) {
        colorAddScaled(rcvrad[0], area_factor * link->K.f, srcrad[0], rcvrad[0]);
    } else {
        int alpha, beta, a, b;
        a = MIN(link->nrcv, rcv->basis_size);
        b = MIN(link->nsrc, link->src->basis_size);
        for ( alpha = 0; alpha < a; alpha++ ) {
            for ( beta = 0; beta < b; beta++ ) {
                colorAddScaled(rcvrad[alpha],
                               area_factor * link->K.p[alpha * link->nsrc + beta],
                               srcrad[beta], rcvrad[alpha]);
            }
        }
    }
}

/**
Requires global variables globalPsrcRad (globalSourceRadiance pointer), globalTheLink (interaction
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

    /* globalTheLink->rcv is a cluster, so it's total area divided by 4 (average projected area)
     * was used to compute link->K. */
    area_factor = surfaceProjectedAreaToSamplePoint(rcv) / (0.25 * globalTheLink->rcv->area);

    doGatherRadiance(rcv, area_factor, globalTheLink, globalPsrcRad);
}

/* Same as above, except that the number of pixels in the scratch frame buffer
 * times the area corresponding one such pixel is used as the visible area
 * of the element. Uses global variables globalPixelArea, globalTheLink, globalPsrcRad. */
static void ZVisSurfaceGatherRadiance(GalerkinElement *rcv) {
    double area_factor;

    if ( rcv->tmp <= 0 ) {
        /* element occupies no pixels in the scratch frame buffer. */
        return;
    }

    area_factor = globalPixelArea * (double) (rcv->tmp) / (0.25 * globalTheLink->rcv->area);
    doGatherRadiance(rcv, area_factor, globalTheLink, globalPsrcRad);

    rcv->tmp = 0;        /* set it to zero for future re-use. */
}

/**
Distributes the source radiance to the surface elements in the
receiver cluster
*/
void
clusterGatherRadiance(INTERACTION *link, COLOR *srcrad) {
    GalerkinElement *src = link->src, *rcv = link->rcv;

    if ( !isCluster(rcv) || src == rcv ) {
        logFatal(-1, "clusterGatherRadiance", "Source and receiver are the same or receiver is not a cluster");
        return;
    }

    globalPsrcRad = srcrad;
    globalTheLink = link;
    globalSamplePoint = galerkinElementMidPoint(src);

    switch ( GLOBAL_galerkin_state.clustering_strategy ) {
        case ISOTROPIC:
            doGatherRadiance(rcv, 1., link, srcrad);
            break;
        case ORIENTED:
            iterateOverSurfaceElementsInCluster(rcv, orientedSurfaceGatherRadiance);
            break;
        case Z_VISIBILITY:
            if ( !OutOfBounds(&globalSamplePoint, rcv->geom->bounds)) {
                iterateOverSurfaceElementsInCluster(rcv, orientedSurfaceGatherRadiance);
            } else {
                float *bbx = ScratchRenderElementPtrs(rcv, globalSamplePoint);

                /* Count how many pixels each element occupies in the scratch frame buffer. */
                ScratchPixelsPerElement();

                /* area corresponding to one pixel on the virtual screen. */
                globalPixelArea = (bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) /
                                  (double) (GLOBAL_galerkin_state.scratch->vp_width * GLOBAL_galerkin_state.scratch->vp_height);

                /* gathers the radiance to each element that occupies at least one
                 * pixel in the scratch frame buffer and sets elem->tmp back to zero
                 * for these element. */
                iterateOverSurfaceElementsInCluster(rcv, ZVisSurfaceGatherRadiance);
            }
            break;
        default:
            logFatal(-1, "clusterGatherRadiance", "Invalid clustering strategy %d",
                     GLOBAL_galerkin_state.clustering_strategy);
    }
}

static void
determineMaxRadiance(GalerkinElement *elem) {
    COLOR rad;
    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
         GLOBAL_galerkin_state.iteration_method == JACOBI ) {
        rad = elem->radiance[0];
    } else {
        rad = elem->unshot_radiance[0];
    }
    colorMaximum(globalSourceRadiance, rad, globalSourceRadiance);
}

COLOR
maxClusterRadiance(GalerkinElement *clus) {
    colorClear(globalSourceRadiance);
    iterateOverSurfaceElementsInCluster(clus, determineMaxRadiance);
    return globalSourceRadiance;
}
