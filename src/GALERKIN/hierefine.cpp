/* hierefine.c: hierarchical refinement */

#include "material/statistics.h"
#include "common/error.h"
#include "skin/Geometry.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/formfactor.h"
#include "GALERKIN/shaftculling.h"
#include "GALERKIN/clustergalerkin.h"

/* ************* Shaftculling stuff for hierarchical refinement *************** */

static GeometryListNode *candlist;    /* candidate occluder list for a pair of patches. */

/* Does shaftculling between elements in a link (if the user asked for it).
 * Updates the global candlist. Returns the old candlist, so it can be restored 
 * later (using UnCull()). */
static GeometryListNode *Cull(INTERACTION *link) {
    GeometryListNode *ocandlist = candlist;

    if ( ocandlist == (GeometryListNode *) nullptr) {
        return (GeometryListNode *) nullptr;
    }

    if ( GLOBAL_galerkin_state.shaftcullmode == DO_SHAFTCULLING_FOR_REFINEMENT ||
         GLOBAL_galerkin_state.shaftcullmode == ALWAYS_DO_SHAFTCULLING ) {
        SHAFT shaft, *the_shaft;

        if ( GLOBAL_galerkin_state.exact_visibility && !IsCluster(link->rcv) && !IsCluster(link->src)) {
            POLYGON rcvpoly, srcpoly;
            the_shaft = ConstructPolygonToPolygonShaft(ElementPolygon(link->rcv, &rcvpoly),
                                                       ElementPolygon(link->src, &srcpoly),
                                                       &shaft);
        } else {
            BOUNDINGBOX srcbounds, rcvbounds;
            the_shaft = ConstructShaft(ElementBounds(link->rcv, rcvbounds),
                                       ElementBounds(link->src, srcbounds), &shaft);
        }
        if ( !the_shaft ) {
            Error("Cull", "Couldn't construct shaft");
            return ocandlist;
        }

        if ( IsCluster(link->rcv)) {
            ShaftDontOpen(&shaft, link->rcv->pog.geom);
        } else {
            ShaftOmit(&shaft, (Geometry *) link->rcv->pog.patch);
        }

        if ( IsCluster(link->src)) {
            ShaftDontOpen(&shaft, link->src->pog.geom);
        } else {
            ShaftOmit(&shaft, (Geometry *) link->src->pog.patch);
        }

        if ( ocandlist == GLOBAL_scene_clusteredWorld ) {
            candlist = ShaftCullGeom(GLOBAL_scene_clusteredWorldGeom, &shaft, (GeometryListNode *) nullptr);
        } else {
            candlist = DoShaftCulling(ocandlist, &shaft, (GeometryListNode *) nullptr);
        }
    }

    return ocandlist;
}

/* Destroys the current candlist and restores the previous one (passed as
 * an argument). */
static void UnCull(GeometryListNode *ocandlist) {
    if ( GLOBAL_galerkin_state.shaftcullmode == DO_SHAFTCULLING_FOR_REFINEMENT ||
         GLOBAL_galerkin_state.shaftcullmode == ALWAYS_DO_SHAFTCULLING ) {
        FreeCandidateList(candlist);
    }

    candlist = ocandlist;
}

/* ********************* Link error estimation *********************** */

static double ColorToError(COLOR rad) {
    RGB rgb;
    ColorToRGB(rad, &rgb);
    return RGBMAXCOMPONENT(rgb);
}

/* Instead of computing the approximation etc... error in radiance or power 
 * error and comparing with a radiance resp. power threshold after weighting 
 * with importance, we modify the threshold and always compare with the error 
 * in radiance norm as if no importance is used. This enables us to skip 
 * estimation of some error terms if it turns out that they are not necessary 
 * anymore. */
static double LinkErrorThreshold(INTERACTION *link, double rcv_area) {
    double threshold = 0.;

    switch ( GLOBAL_galerkin_state.error_norm ) {
        case RADIANCE_ERROR:
            threshold = ColorToError(GLOBAL_statistics_maxSelfEmittedRadiance) * GLOBAL_galerkin_state.rel_link_error_threshold;
            break;
        case POWER_ERROR:
            threshold = ColorToError(GLOBAL_statistics_maxSelfEmittedPower) * GLOBAL_galerkin_state.rel_link_error_threshold / (M_PI * rcv_area);
            break;
        default:
            Fatal(2, "EvaluateInteraction", "Invalid error norm");
    }

    /* Weight the error with the potential of the receiver in case of view-potential
     * driven gathering (potential is used in a totally different way in shooting
     * algorithms). It is assumed that the average direct potential is about half
     * of the maximum direct potential. This way, about an equal precision is
     * obtained in the visible parts of the scene whether or not importance is used. */
    /* instead of weighting the error, we weight the threshold with the inverse. */
    if ( GLOBAL_galerkin_state.importance_driven &&
         (GLOBAL_galerkin_state.iteration_method == JACOBI ||
          GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL)) {
        threshold /= 2. * link->rcv->potential.f / GLOBAL_statistics_maxDirectPotential;
    }

    return threshold;
}

/* Compute an estimate for the approximation error that would be made if the
 * candidate link were used for light transport. Use the sources unshot 
 * radiance when doing shooting and the total radiance when gathering. */
static double ApproximationError(INTERACTION *link, COLOR srcrho, COLOR rcvrho, double rcv_area) {
    COLOR error, srcrad;
    double approx_error = 0., approx_error2;

    switch ( GLOBAL_galerkin_state.iteration_method ) {
        case GAUSS_SEIDEL:
        case JACOBI:
            if ( IsCluster(link->src) && link->src != link->rcv ) {
                srcrad = MaxClusterRadiance(link->src); /* SourceClusterRadiance(link); */
            } else {
                srcrad = link->src->radiance[0];
            }

            COLORPRODSCALED(rcvrho, link->deltaK.f, srcrad, error);
            COLORABS(error, error);
            approx_error = ColorToError(error);
            break;

        case SOUTHWELL:
            if ( IsCluster(link->src) && link->src != link->rcv ) {
                srcrad = SourceClusterRadiance(link); /* returns unshot radiance for shooting */
            } else {
                srcrad = link->src->unshot_radiance[0];
            }

            COLORPRODSCALED(rcvrho, link->deltaK.f, srcrad, error);
            COLORABS(error, error);
            approx_error = ColorToError(error);

            if ( GLOBAL_galerkin_state.importance_driven && IsCluster(link->rcv)) {
                /* make sure the link is also suited for transport of unshot potential
                 * from source to receiver. Note that it makes no sense to
                 * subdivide receiver patches (potential is only used to help
                 * choosing a radiance shooting patch. */
                approx_error2 = (ColorToError(srcrho) * link->deltaK.f * link->src->unshot_potential.f);

                /* compare potential error w.r.t. maximum direct potential or importance
                 * instead of selfemitted radiance or power. */
                switch ( GLOBAL_galerkin_state.error_norm ) {
                    case RADIANCE_ERROR:
                        approx_error2 *= ColorToError(GLOBAL_statistics_maxSelfEmittedRadiance) / GLOBAL_statistics_maxDirectPotential;
                        break;
                    case POWER_ERROR:
                        approx_error2 *= ColorToError(GLOBAL_statistics_maxSelfEmittedPower) / M_PI / GLOBAL_statistics_maxDirectImportance;
                        break;
                }
                if ( approx_error2 > approx_error ) {
                    approx_error = approx_error2;
                }
            }
            break;

        default:
            Fatal(-1, "ApproximationError", "Invalid iteration method");
    }

    return approx_error;
}

/* Estimates the error due to the variation of the source cluster radiance 
 * as seen from a number of sample points on the receiver element. Especially
 * when intra source cluster visibility is handled with a Z-buffer algorithm,
 * this operation is quite expensive and should be avoided when not stricktly
 * necessary. */
static double SourceClusterRadianceVariationError(INTERACTION *link, COLOR rcvrho, double rcv_area) {
    Vector3D rcverts[8];
    int i, nrcverts;
    COLOR minsrcrad, maxsrcrad, error;
    double K;

    K = (link->nsrc == 1 && link->nrcv == 1) ? link->K.f : link->K.p[0];
    if ( K == 0. || COLORNULL(rcvrho) || COLORNULL(link->src->radiance[0])) {
        /* receiver reflectivity or coupling coefficient or source radiance
         * is zero */
        return 0.;
    }

    nrcverts = ElementVertices(link->rcv, rcverts);

    COLORSETMONOCHROME(minsrcrad, HUGE);
    COLORSETMONOCHROME(maxsrcrad, -HUGE);
    for ( i = 0; i < nrcverts; i++ ) {
        COLOR rad;
        rad = ClusterRadianceToSamplePoint(link->src, rcverts[i]);
        COLORMIN(minsrcrad, rad, minsrcrad);
        COLORMAX(maxsrcrad, rad, maxsrcrad);
    }
    COLORSUBTRACT(maxsrcrad, minsrcrad, error);

    COLORPRODSCALED(rcvrho, K / rcv_area, error, error);
    COLORABS(error, error);
    return ColorToError(error);
}

/* Evaluates the interaction and returns a code telling whether it is accurate enough
 * for computing light transport, or what to do in order to reduce the
 * (estimated) error in the most efficient way. This is the famous oracle function
 * which is so crucial for efficient hierarchical refinement. 
 *
 * See DOC/galerkin.text.
 */

enum INTERACTION_EVALUATION_CODE {
    ACCURATE_ENOUGH,
    REGULAR_SUBDIVIDE_SOURCE, REGULAR_SUBDIVIDE_RECEIVER,
    SUBDIVIDE_SOURCE_CLUSTER, SUBDIVIDE_RECEIVER_CLUSTER,
    ENLARGE_RECEIVER_BASIS
};

static INTERACTION_EVALUATION_CODE EvaluateInteraction(INTERACTION *link) {
    COLOR srcrho, rcvrho;
    double error, threshold, rcv_area, min_area;
    INTERACTION_EVALUATION_CODE code = ACCURATE_ENOUGH;

    if ( !GLOBAL_galerkin_state.hierarchical ) {
        return ACCURATE_ENOUGH;
    }    /* simply don't refine. */

    /* determine receiver area (projected visible area for a receiver cluster)
     * and reflectivity. */
    if ( IsCluster(link->rcv)) {
        COLORSETMONOCHROME(rcvrho, 1.);
        rcv_area = ReceiverClusterArea(link);
    } else {
        rcvrho = REFLECTIVITY(link->rcv->pog.patch);
        rcv_area = link->rcv->area;
    }

    /* determine source reflectivity. */
    if ( IsCluster(link->src)) {
        COLORSETMONOCHROME(srcrho, 1.);
    } else
        srcrho = REFLECTIVITY(link->src->pog.patch);

    /* determine error estimate and error threshold */
    threshold = LinkErrorThreshold(link, rcv_area);
    error = ApproximationError(link, srcrho, rcvrho, rcv_area);

    if ( IsCluster(link->src) && error < threshold && GLOBAL_galerkin_state.clustering_strategy != ISOTROPIC )
        error += SourceClusterRadianceVariationError(link, rcvrho, rcv_area);

    /* Minimal element area for which subdivision is allowed. */
    min_area = GLOBAL_statistics_totalArea * GLOBAL_galerkin_state.rel_min_elem_area;

    code = ACCURATE_ENOUGH;
    if ( error > threshold ) {
        /* A very simple but robust subdivision strategy: subdivide the
         * largest of the two elements in order to reduce the error. */
        if ((!(IsCluster(link->src) && IsLightSource(link->src))) &&
            (rcv_area > link->src->area)) {
            if ( rcv_area > min_area ) {
                if ( IsCluster(link->rcv))
                    code = SUBDIVIDE_RECEIVER_CLUSTER;
                else
                    code = REGULAR_SUBDIVIDE_RECEIVER;
            }
        } else {
            if ( IsCluster(link->src))
                code = SUBDIVIDE_SOURCE_CLUSTER;
            else if ( link->src->area > min_area )
                code = REGULAR_SUBDIVIDE_SOURCE;
        }
    }

    return code;
}

/* ****************** Light transport computation ********************* */

/* Computes light transport over the given interaction, which is supposed to be
 * accurate enough for doing so. Renormalisation and reflection and such is done 
 * once for all accumulated received radiance during push-pull. */
static void ComputeLightTransport(INTERACTION *link) {
    COLOR *srcrad, *rcvrad, avsrclusrad;
    int alpha, beta, a, b;

    /* Update the number of effectively used radiance coefficients on the
     * receiver element. */
    a = MIN(link->nrcv, link->rcv->basis_size);
    b = MIN(link->nsrc, link->src->basis_size);
    if ( a > link->rcv->basis_used ) {
        link->rcv->basis_used = a;
    }
    if ( b > link->src->basis_used ) {
        link->src->basis_used = b;
    }

    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        srcrad = link->src->unshot_radiance;
    } else {
        srcrad = link->src->radiance;
    }

    if ( IsCluster(link->src) && link->src != link->rcv ) {
        avsrclusrad = SourceClusterRadiance(link);
        srcrad = &avsrclusrad;
    }

    if ( IsCluster(link->rcv) && link->src != link->rcv ) {
        ClusterGatherRadiance(link, srcrad);
    } else {
        rcvrad = link->rcv->received_radiance;
        if ( link->nrcv == 1 && link->nsrc == 1 ) {
            COLORADDSCALED(rcvrad[0], link->K.f, srcrad[0], rcvrad[0]);
        } else {
            for ( alpha = 0; alpha < a; alpha++ ) {
                for ( beta = 0; beta < b; beta++ ) COLORADDSCALED(rcvrad[alpha], link->K.p[alpha * link->nsrc + beta],
                                                                  srcrad[beta], rcvrad[alpha])
            };
        }
    }

    if ( GLOBAL_galerkin_state.importance_driven ) {
        float K = ((link->nrcv == 1 && link->nsrc == 1) ? link->K.f : link->K.p[0]);
        COLOR rcvrho, srcrho;

        if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
             GLOBAL_galerkin_state.iteration_method == JACOBI ) {
            if ( IsCluster(link->rcv)) {
                COLORSETMONOCHROME(rcvrho, 1.);
            } else {
                rcvrho = REFLECTIVITY(link->rcv->pog.patch);
            }
            link->src->received_potential.f += K * ColorToError(rcvrho) * link->rcv->potential.f;
        } else if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
            if ( IsCluster(link->src)) {
                COLORSETMONOCHROME(srcrho, 1.);
            } else {
                srcrho = REFLECTIVITY(link->src->pog.patch);
            }
            link->rcv->received_potential.f += K * ColorToError(srcrho) * link->src->unshot_potential.f;
        } else {
            Fatal(-1, "ComputeLightTransport", "Hela hola did you introduce a new iteration method or so??");
        }
    }
}

/* ********************** Refinement procedures ************************* */

/* Computes the formfactor and error esitmation coefficients. If the formfactor
 * is not zero, the data is filled in the INTERACTION pointed to by 'link'
 * and true is returned. If the elements don't itneract, false is returned. */
int CreateSubdivisionLink(ELEMENT *rcv, ELEMENT *src, INTERACTION *link) {
    link->rcv = rcv;
    link->src = src;

    /* Always a constant approximation on cluster elements. */
    if ( IsCluster(link->rcv)) {
        link->nrcv = 1;
    } else {
        link->nrcv = rcv->basis_size;
    }

    if ( IsCluster(link->src)) {
        link->nsrc = 1;
    } else {
        link->nsrc = src->basis_size;
    }

    AreaToAreaFormFactor(link, candlist);

    return link->vis != 0;
}

/* Duplicates the INTERACTION data and stores it with the receivers interactions
 * if doing gathering and with the source for shooting. */
static void StoreInteraction(INTERACTION *link) {
    ELEMENT *src = link->src, *rcv = link->rcv;

    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        src->interactions = InteractionListAdd(src->interactions, InteractionDuplicate(link));
    } else {
        rcv->interactions = InteractionListAdd(rcv->interactions, InteractionDuplicate(link));
    }
}

static int RefineRecursive(INTERACTION *link);    /* forward decl. */

/* Subdivides the source element, creates subinteractions and refines. If the 
 * subinteractions do not need to be refined any further, they are added to
 * either the sources, either the receivers interaction list, depending on the 
 * iteration method being used. This routine always returns true indicating that
 * the passed interaction is always replaced by lower level interactions. */
static int RegularSubdivideSource(INTERACTION *link) {
    GeometryListNode *ocandlist = Cull(link);
    ELEMENT *src = link->src, *rcv = link->rcv;
    int i;

    RegularSubdivideElement(src);
    for ( i = 0; i < 4; i++ ) {
        ELEMENT *child = src->regular_subelements[i];
        INTERACTION subinteraction;
        float ff[MAXBASISSIZE * MAXBASISSIZE];
        subinteraction.K.p = ff;    /* temporary storage for the formfactors */
        /* subinteraction.deltaK.p = */

        if ( CreateSubdivisionLink(rcv, child, &subinteraction)) {
            if ( !RefineRecursive(&subinteraction)) {
                StoreInteraction(&subinteraction);
            }
        }
    }

    UnCull(ocandlist);
    return true;
}

/* Same, but subdivides the receiver element. */
static int RegularSubdivideReceiver(INTERACTION *link) {
    GeometryListNode *ocandlist = Cull(link);
    ELEMENT *src = link->src, *rcv = link->rcv;
    int i;

    RegularSubdivideElement(rcv);
    for ( i = 0; i < 4; i++ ) {
        INTERACTION subinteraction;
        float ff[MAXBASISSIZE * MAXBASISSIZE];
        ELEMENT *child = rcv->regular_subelements[i];
        subinteraction.K.p = ff;

        if ( CreateSubdivisionLink(child, src, &subinteraction)) {
            if ( !RefineRecursive(&subinteraction)) {
                StoreInteraction(&subinteraction);
            }
        }
    }

    UnCull(ocandlist);
    return true;
}

/* Replace the interaction by interactions with the subclusters of the source,
 * which is a cluster. */
static int SubdivideSourceCluster(INTERACTION *link) {
    GeometryListNode *ocandlist = Cull(link);
    ELEMENT *src = link->src, *rcv = link->rcv;
    ELEMENTLIST *subcluslist;

    for ( subcluslist = src->irregular_subelements; subcluslist; subcluslist = subcluslist->next ) {
        ELEMENT *child = subcluslist->element;
        INTERACTION subinteraction;
        float ff[MAXBASISSIZE * MAXBASISSIZE];
        subinteraction.K.p = ff;    /* temporary storage for the formfactors */
        /* subinteraction.deltaK.p = */

        if ( !IsCluster(child)) {
            PATCH *the_patch = child->pog.patch;
            if ((IsCluster(rcv) &&
                 BoundsBehindPlane(geomBounds(rcv->pog.geom), &the_patch->normal, the_patch->plane_constant)) ||
                (!IsCluster(rcv) && !Facing(rcv->pog.patch, the_patch))) {
                continue;
            }
        }

        if ( CreateSubdivisionLink(rcv, child, &subinteraction)) {
            if ( !RefineRecursive(&subinteraction)) {
                StoreInteraction(&subinteraction);
            }
        }
    }

    UnCull(ocandlist);
    return true;
}

/* Replace the interaction by interactions with the subclusters of the receiver,
 * which is a cluster. */
static int SubdivideReceiverCluster(INTERACTION *link) {
    GeometryListNode *ocandlist = Cull(link);
    ELEMENT *src = link->src, *rcv = link->rcv;
    ELEMENTLIST *subcluslist;

    for ( subcluslist = rcv->irregular_subelements; subcluslist; subcluslist = subcluslist->next ) {
        ELEMENT *child = subcluslist->element;
        INTERACTION subinteraction;
        float ff[MAXBASISSIZE * MAXBASISSIZE];
        subinteraction.K.p = ff;

        if ( !IsCluster(child)) {
            PATCH *the_patch = child->pog.patch;
            if ((IsCluster(src) &&
                 BoundsBehindPlane(geomBounds(src->pog.geom), &the_patch->normal, the_patch->plane_constant)) ||
                (!IsCluster(src) && !Facing(src->pog.patch, the_patch))) {
                continue;
            }
        }

        if ( CreateSubdivisionLink(child, src, &subinteraction)) {
            if ( !RefineRecursive(&subinteraction)) {
                StoreInteraction(&subinteraction);
            }
        }
    }

    UnCull(ocandlist);
    return true;
}

/* Recursievly refines the interaction. Returns true if the interaction was 
 * effectively refined, so the specified interaction can be deleted. Returns false
 * if the interaction was not refined and is to be retained. If the interaction
 * does not need to be refined, light transport over the interaction is computed. */
int RefineRecursive(INTERACTION *link) {
    int refined = false;

    switch ( EvaluateInteraction(link)) {
        case ACCURATE_ENOUGH:
            ComputeLightTransport(link);
            refined = false;
            break;
        case REGULAR_SUBDIVIDE_SOURCE:
            refined = RegularSubdivideSource(link);
            break;
        case REGULAR_SUBDIVIDE_RECEIVER:
            refined = RegularSubdivideReceiver(link);
            break;
        case SUBDIVIDE_SOURCE_CLUSTER:
            refined = SubdivideSourceCluster(link);
            break;
        case SUBDIVIDE_RECEIVER_CLUSTER:
            refined = SubdivideReceiverCluster(link);
            break;
        default:
            Fatal(2, "RefineRecursive", "Invalid result from EvaluateInteraction()");
    }

    return refined;
}

void RefineInteraction(INTERACTION *link) {
    candlist = GLOBAL_scene_clusteredWorld;    /* candidate occluder list for a pair of patches. */
    if ( GLOBAL_galerkin_state.exact_visibility && link->vis == 255 ) {
        candlist = (GeometryListNode *) nullptr;
    } /* we know for sure that there is full visibility */

    if ( RefineRecursive(link)) {
        if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL )
            link->src->interactions = InteractionListRemove(link->src->interactions, link);
        else
            link->rcv->interactions = InteractionListRemove(link->rcv->interactions, link);
        InteractionDestroy(link);
    }
}

/* Refines and computes light transport over all interactions of the given
 * toplevel element. */
void RefineInteractions(ELEMENT *top) {
    /* interactions will only be replaced by lower level interactions. We try refinement
     * beginning at the lowest levels in the hierarchy and working upwards to
     * prevent already refined interactions from being tested for refinement
     * again. */
    ITERATE_IRREGULAR_SUBELEMENTS(top, RefineInteractions);
    ITERATE_REGULAR_SUBELEMENTS(top, RefineInteractions);

    /* Iterate over the interactions. Interactions that are refined are removed from the
     * list in RefineInteraction(). ListIterate allows the current element to be deleted. */
    InteractionListIterate(top->interactions, RefineInteraction);
}
