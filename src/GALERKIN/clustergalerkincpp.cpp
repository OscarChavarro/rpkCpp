/* cluster-specific operations */

#include "common/error.h"
#include "skin/Geometry.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/coefficientsgalerkin.h"
#include "GALERKIN/scratch.h"
#include "GALERKIN/mrvisibility.h"

static ELEMENT *GalerkinDoCreateClusterHierarchy(Geometry *geom);

/* Creates a cluster hierarchy for the Geometry and adds it to the subcluster list of the
 * given parent cluster */
static void GeomAddClusterChild(Geometry *geom, ELEMENT *parent_cluster) {
    ELEMENT *clus;

    clus = GalerkinDoCreateClusterHierarchy(geom);

    parent_cluster->irregular_subelements = ElementListAdd(parent_cluster->irregular_subelements, clus);
    clus->parent = parent_cluster;
}

/* Adds the toplevel (surace) element of the patch to the list of irregular
 * subelements of the cluster. */
static void PatchAddClusterChild(PATCH *patch, ELEMENT *cluster) {
    ELEMENT *surfel = (ELEMENT *) (patch->radiance_data);

    cluster->irregular_subelements = ElementListAdd(cluster->irregular_subelements, surfel);
    surfel->parent = cluster;
}

/* Initializes the cluster element. Called bottom-up: first the
 * lowest level clusters and so up. */
static void ClusterInit(ELEMENT *clus) {
    ELEMENTLIST *subellist;

    /* total area of surfaces inside the cluster is sum of the areas of
     * the subclusters + pull radiance. */
    clus->area = 0.;
    clus->nrpatches = 0;
    clus->minarea = HUGE;
    CLEARCOEFFICIENTS(clus->radiance, clus->basis_size);
    for ( subellist = clus->irregular_subelements; subellist; subellist = subellist->next ) {
        ELEMENT *subclus = subellist->element;
        clus->area += subclus->area;
        clus->nrpatches += subclus->nrpatches;
        colorAddScaled(clus->radiance[0], subclus->area, subclus->radiance[0], clus->radiance[0]);
        if ( subclus->minarea < clus->minarea ) {
            clus->minarea = subclus->minarea;
        }
        clus->flags |= (subclus->flags & IS_LIGHT_SOURCE);
        colorAddScaled(clus->Ed, subclus->area, subclus->Ed, clus->Ed);
    }
    colorScale((1. / clus->area), clus->radiance[0], clus->radiance[0]);
    colorScale((1. / clus->area), clus->Ed, clus->Ed);

    /* also pull unshot radiance for the "shooting" methods */
    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        CLEARCOEFFICIENTS(clus->unshot_radiance, clus->basis_size);
        for ( subellist = clus->irregular_subelements; subellist; subellist = subellist->next ) {
            ELEMENT *subclus = subellist->element;
            colorAddScaled(clus->unshot_radiance[0], subclus->area, subclus->unshot_radiance[0],
                           clus->unshot_radiance[0]);
        }
        colorScale((1. / clus->area), clus->unshot_radiance[0], clus->unshot_radiance[0]);
    }

    /* compute equivalent blocker (or blocker complement) size for multiresolution
     * visibility */
    /* if (GLOBAL_galerkin_state.multires_visibility) */ {
        /* This is very costly
        clus->bsize = GeomBlockerSize(clus->pog.geom);
        */
        float *bbx = clus->pog.geom->bounds;
        clus->bsize = MAX((bbx[MAX_X] - bbx[MIN_X]), (bbx[MAX_Y] - bbx[MIN_Y]));
        clus->bsize = MAX(clus->bsize, (bbx[MAX_Z] - bbx[MIN_Z]));
    } /* else
    clus->bsize = 2.*sqrt((clus->area/4.)/M_PI); */
}

/* Creates a cluster for the Geometry, recurses for the children GEOMs, initializes and
 * returns the created cluster. */
static ELEMENT *GalerkinDoCreateClusterHierarchy(Geometry *geom) {
    ELEMENT *clus;

    /* geom will be nullptr if e.g. no scene is loaded when selecting
     * Galerkin radiosity for radiance computations. */
    if ( !geom ) {
        return (ELEMENT *) nullptr;
    }

    /* create a cluster for the geom */
    clus = CreateClusterElement(geom);
    geom->radiance_data = (void *) clus;

    /* recursively creates list of subclusters */
    if ( geomIsAggregate(geom)) {
        GeomListIterate1A(geomPrimList(geom), (void (*)(Geometry *, void *)) GeomAddClusterChild, (void *) clus);
    } else {
        PatchListIterate1A(geomPatchList(geom), (void (*)(PATCH *, void *)) PatchAddClusterChild, (void *) clus);
    }

    ClusterInit(clus);

    return clus;
}

/* First initializes for equivalent blocker size determination, then calls
 * the above function, and finally terminates equivalent blocker size 
 * determination. */
ELEMENT *galerkinCreateClusterHierarchy(Geometry *geom) {
    ELEMENT *clus;

    BlockerInit();
    clus = GalerkinDoCreateClusterHierarchy(geom);
    BlockerTerminate();

    return clus;
}

/* Disposes of the cluster hierarchy */
void galerkinDestroyClusterHierarchy(ELEMENT *cluster) {
    if ( !cluster || !IsCluster(cluster)) {
        return;
    }

    ITERATE_IRREGULAR_SUBELEMENTS(cluster, galerkinDestroyClusterHierarchy);
    DestroyClusterElement(cluster);
}

/* Executes func for every surface element in the cluster. */
void iterateOverSurfaceElementsInCluster(ELEMENT *clus, void (*func)(ELEMENT *elem)) {
    if ( !IsCluster(clus)) {
        func(clus);
    } else {
        ELEMENTLIST *subcluslist;
        for ( subcluslist = clus->irregular_subelements; subcluslist; subcluslist = subcluslist->next ) {
            ELEMENT *subclus = subcluslist->element;
            iterateOverSurfaceElementsInCluster(subclus, func);
        }
    }
}

static COLOR srcrad;
static Vector3D samplepoint;

/* Uses global variables srcrad and samplepoint: accumulates the
 * power emitted by the element towards the samplepoint in srcrad
 * only taking into account the surface orientation w.r.t. the 
 * sample point, (ignores intra cluster visibility). */
static void AccumulatePowerToSamplePoint(ELEMENT *src) {
    double srcos, dist;
    Vector3D dir;
    COLOR rad;

    VECTORSUBTRACT(samplepoint, src->pog.patch->midpoint, dir);
    dist = VECTORNORM(dir);
    if ( dist < EPSILON ) {
        srcos = 1.;
    } else {
        srcos = VECTORDOTPRODUCT(dir, src->pog.patch->normal) / dist;
    }
    if ( srcos <= 0. ) {
        return;
    }    /* receiver point is behind the src */

    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
         GLOBAL_galerkin_state.iteration_method == JACOBI ) {
        rad = src->radiance[0];
    } else {
        rad = src->unshot_radiance[0];
    }

    colorAddScaled(srcrad, srcos * src->area, rad, srcrad);
}

/* Returns the radiance or unshot radiance (depending on the
 * iteration method) emitted by the src cluster,
 * towards the sample point. */
COLOR clusterRadianceToSamplePoint(ELEMENT *src, Vector3D sample) {
    switch ( GLOBAL_galerkin_state.clustering_strategy ) {
        case ISOTROPIC:
            return src->radiance[0];

        case ORIENTED: {
            samplepoint = sample;

            /* accumulate the power emitted by the patches in the source cluster
             * towards the sample point. */
            colorClear(srcrad);
            iterateOverSurfaceElementsInCluster(src, AccumulatePowerToSamplePoint);

            /* divide by the source area used for computing the form factor:
             * src->area / 4. (average projected area) */
            colorScale(4. / src->area, srcrad, srcrad);
            return srcrad;
        }

        case Z_VISIBILITY:
            if ( !IsCluster(src) || !OutOfBounds(&sample, src->pog.geom->bounds)) {
                return src->radiance[0];
            } else {
                double areafactor;

                /* Render pointers to the elements in the source cluster into the scratch frame
                 * buffer as seen from the sample point. */
                float *bbx = ScratchRenderElementPtrs(src, sample);

                /* Compute average radiance on the virtual screen */
                srcrad = ScratchRadiance();

                /* areafactor = area of virtual screen / source cluster area used for
                 * form factor computation */
                areafactor = ((bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y])) /
                             (0.25 * src->area);
                colorScale(areafactor, srcrad, srcrad);
                return srcrad;
            }

        default:
            Fatal(-1, "clusterRadianceToSamplePoint", "Invalid clustering strategy %d\n", GLOBAL_galerkin_state.clustering_strategy);
    }

    return srcrad;    /* this point is never reached */
}

/* Determines the average radiance or unshot radiance (depending on
 * the iteration method) emitted by the source cluster towards the 
 * receiver in the link. The source should be a cluster. */
COLOR sourceClusterRadiance(INTERACTION *link) {
    ELEMENT *src = link->src, *rcv = link->rcv;

    if ( !IsCluster(src) || src == rcv ) {
        Fatal(-1, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    /* take a sample point on the receiver */
    return clusterRadianceToSamplePoint(src, ElementMidpoint(rcv));
}

/* computes projected area of receiver surface element towards the sample point
 * (global variable). Ignores intra cluster visibility. */
static double SurfaceProjectedAreaToSamplePoint(ELEMENT *rcv) {
    double rcvcos, dist;
    Vector3D dir;

    VECTORSUBTRACT(samplepoint, rcv->pog.patch->midpoint, dir);
    dist = VECTORNORM(dir);
    if ( dist < EPSILON ) {
        rcvcos = 1.;
    } else {
        rcvcos = VECTORDOTPRODUCT(dir, rcv->pog.patch->normal) / dist;
    }
    if ( rcvcos <= 0. ) {
        return 0.;
    }    /* sample point is behind the rcv */

    return rcvcos * rcv->area;
}

static double proj_area;

static void AccumulateProjectedAreaToSamplePoint(ELEMENT *rcv) {
    proj_area += SurfaceProjectedAreaToSamplePoint(rcv);
}

/* Computes projected area of receiver cluster as seen from the midpoint of the source,
 * ignoring intra-receiver visibility. */
double receiverClusterArea(INTERACTION *link) {
    ELEMENT *src = link->src, *rcv = link->rcv;

    if ( !IsCluster(rcv) || src == rcv ) {
        return rcv->area;
    }

    switch ( GLOBAL_galerkin_state.clustering_strategy ) {
        case ISOTROPIC:
            return rcv->area;

        case ORIENTED: {
            samplepoint = ElementMidpoint(src);
            proj_area = 0.;
            iterateOverSurfaceElementsInCluster(rcv, AccumulateProjectedAreaToSamplePoint);
            return proj_area;
        }

        case Z_VISIBILITY:
            samplepoint = ElementMidpoint(src);
            if ( !OutOfBounds(&samplepoint, rcv->pog.geom->bounds)) {
                return rcv->area;
            } else {
                float *bbx = ScratchRenderElementPtrs(rcv, samplepoint);

                /* projected area is the number of non background pixels over
                 * the total number of pixels * area of the virtual screen. */
                proj_area = (double) ScratchNonBackgroundPixels() *
                            (bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) /
                            (double) (GLOBAL_galerkin_state.scratch->vp_width * GLOBAL_galerkin_state.scratch->vp_height);
                return proj_area;
            }

        default:
            Fatal(-1, "receiverClusterArea", "Invalid clustering strategy %d", GLOBAL_galerkin_state.clustering_strategy);
    }

    return 0.;    /* this point is never reached and if it is it's your fault. */
}

/* Gathers radiance over the interaction, from the interaction source to
 * the specified receiver element. area_factor is a correction factor
 * to account for the fact that the receiver cluster area/4 was used in
 * form factor computations instead of the true receiver area. The source
 * radiance is explicitely given. */
static void DoGatherRadiance(ELEMENT *rcv, double area_factor, INTERACTION *link, COLOR *srcrad) {
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

static COLOR *psrcrad;
static INTERACTION *the_link;

/* Requires global variables psrcrad (srcrad pointer), the_link (interaction
 * over which is being gathered) and samplepoint (midpoint of source 
 * element). rcv is a surface element belonging to the receiver cluster
 * in the interaction. This routines gathers radiance to this receiver
 * surface, taking into account the projected area of the receiver
 * towards the midpoint of the source, ignoring visibility in the receiver 
 * cluster. */
static void OrientedSurfaceGatherRadiance(ELEMENT *rcv) {
    double area_factor;

    /* the_link->rcv is a cluster, so it's total area divided by 4 (average projected area)
     * was used to compute link->K. */
    area_factor = SurfaceProjectedAreaToSamplePoint(rcv) / (0.25 * the_link->rcv->area);

    DoGatherRadiance(rcv, area_factor, the_link, psrcrad);
}

/* area corresponding to one pixel in the scratch frame buffer. */
static double pix_area;

/* Same as above, except that the number of pixels in the scratch frame buffer
 * times the area corresponding one such pixel is used as the visible area
 * of the element. Uses global variables pix_area, the_link, psrcrad. */
static void ZVisSurfaceGatherRadiance(ELEMENT *rcv) {
    double area_factor;

    if ( rcv->tmp <= 0 ) {
        /* element occupies no pixels in the scratch frame buffer. */
        return;
    }

    area_factor = pix_area * (double) (rcv->tmp) / (0.25 * the_link->rcv->area);
    DoGatherRadiance(rcv, area_factor, the_link, psrcrad);

    rcv->tmp = 0;        /* set it to zero for future re-use. */
}

/* Distributes the srcrad radiance to the surface elements in the
 * receiver cluster */
void clusterGatherRadiance(INTERACTION *link, COLOR *srcrad) {
    ELEMENT *src = link->src, *rcv = link->rcv;

    if ( !IsCluster(rcv) || src == rcv ) {
        Fatal(-1, "clusterGatherRadiance", "Source and receiver are the same or receiver is not a cluster");
        return;
    }

    psrcrad = srcrad;
    the_link = link;
    samplepoint = ElementMidpoint(src);

    switch ( GLOBAL_galerkin_state.clustering_strategy ) {
        case ISOTROPIC:
            DoGatherRadiance(rcv, 1., link, srcrad);
            break;
        case ORIENTED:
            iterateOverSurfaceElementsInCluster(rcv, OrientedSurfaceGatherRadiance);
            break;
        case Z_VISIBILITY:
            if ( !OutOfBounds(&samplepoint, rcv->pog.geom->bounds)) {
                iterateOverSurfaceElementsInCluster(rcv, OrientedSurfaceGatherRadiance);
            } else {
                float *bbx = ScratchRenderElementPtrs(rcv, samplepoint);

                /* Count how many pixels each element occupies in the scratch frame buffer. */
                ScratchPixelsPerElement();

                /* area corresponding to one pixel on the virtual screen. */
                pix_area = (bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) /
                           (double) (GLOBAL_galerkin_state.scratch->vp_width * GLOBAL_galerkin_state.scratch->vp_height);

                /* gathers the radiance to each element that occupies at least one
                 * pixel in the scratch frame buffer and sets elem->tmp back to zero
                 * for these element. */
                iterateOverSurfaceElementsInCluster(rcv, ZVisSurfaceGatherRadiance);
            }
            break;
        default:
            Fatal(-1, "clusterGatherRadiance", "Invalid clustering strategy %d", GLOBAL_galerkin_state.clustering_strategy);
    }
}

static void DetermineMaxRadiance(ELEMENT *elem) {
    COLOR rad;
    if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
         GLOBAL_galerkin_state.iteration_method == JACOBI ) {
        rad = elem->radiance[0];
    } else {
        rad = elem->unshot_radiance[0];
    }
    colorMaximum(srcrad, rad, srcrad);
}

COLOR maxClusterRadiance(ELEMENT *clus) {
    colorClear(srcrad);
    iterateOverSurfaceElementsInCluster(clus, DetermineMaxRadiance);
    return srcrad;
}
