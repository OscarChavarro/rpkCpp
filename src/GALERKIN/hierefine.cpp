/**
Hierarchical refinement
*/

#include "common/error.h"
#include "material/statistics.h"
#include "skin/Geometry.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/formfactor.h"
#include "GALERKIN/shaftculling.h"
#include "GALERKIN/clustergalerkincpp.h"

/**
Shaftculling stuff for hierarchical refinement
*/

static int refineRecursive(INTERACTION *link);    /* forward decl. */

static GeometryListNode *globalCandidatesList;    /* candidate occluder list for a pair of patches. */

/**
Evaluates the interaction and returns a code telling whether it is accurate enough
for computing light transport, or what to do in order to reduce the
(estimated) error in the most efficient way. This is the famous oracle function
which is so crucial for efficient hierarchical refinement.

See DOC/galerkin.text.
*/
enum INTERACTION_EVALUATION_CODE {
    ACCURATE_ENOUGH,
    REGULAR_SUBDIVIDE_SOURCE,
    REGULAR_SUBDIVIDE_RECEIVER,
    SUBDIVIDE_SOURCE_CLUSTER,
    SUBDIVIDE_RECEIVER_CLUSTER,
    ENLARGE_RECEIVER_BASIS
};

/**
Does shaft-culling between elements in a link (if the user asked for it).
Updates the global candlist. Returns the old candidate list, so it can be restored
later (using hierarchicRefinementUnCull())
*/
static GeometryListNode *
hierarchicRefinementCull(INTERACTION *link) {
    GeometryListNode *ocandlist = globalCandidatesList;

    if ( ocandlist == (GeometryListNode *) nullptr) {
        return (GeometryListNode *) nullptr;
    }

    if ( GLOBAL_galerkin_state.shaftcullmode == DO_SHAFTCULLING_FOR_REFINEMENT ||
         GLOBAL_galerkin_state.shaftcullmode == ALWAYS_DO_SHAFTCULLING ) {
        SHAFT shaft, *the_shaft;

        if ( GLOBAL_galerkin_state.exact_visibility && !isCluster(link->receiverElement) && !isCluster(link->sourceElement)) {
            POLYGON rcvpoly, srcpoly;
            the_shaft = constructPolygonToPolygonShaft(galerkinElementPolygon(link->receiverElement, &rcvpoly),
                                                       galerkinElementPolygon(link->sourceElement, &srcpoly),
                                                       &shaft);
        } else {
            BOUNDINGBOX srcbounds, rcvbounds;
            the_shaft = constructShaft(galerkinElementBounds(link->receiverElement, rcvbounds),
                                       galerkinElementBounds(link->sourceElement, srcbounds), &shaft);
        }
        if ( !the_shaft ) {
            logError("hierarchicRefinementCull", "Couldn't construct shaft");
            return ocandlist;
        }

        if ( isCluster(link->receiverElement)) {
            shaftDontOpen(&shaft, link->receiverElement->geom);
        } else {
            shaftOmit(&shaft, (Geometry *) link->receiverElement->patch);
        }

        if ( isCluster(link->sourceElement)) {
            shaftDontOpen(&shaft, link->sourceElement->geom);
        } else {
            shaftOmit(&shaft, (Geometry *) link->sourceElement->patch);
        }

        if ( ocandlist == GLOBAL_scene_clusteredWorld ) {
            globalCandidatesList = shaftCullGeom(GLOBAL_scene_clusteredWorldGeom, &shaft, (GeometryListNode *) nullptr);
        } else {
            globalCandidatesList = doShaftCulling(ocandlist, &shaft, (GeometryListNode *) nullptr);
        }
    }

    return ocandlist;
}

/**
Destroys the current candlist and restores the previous one (passed as
an argument)
*/
static void
hierarchicRefinementUnCull(GeometryListNode *ocandlist) {
    if ( GLOBAL_galerkin_state.shaftcullmode == DO_SHAFTCULLING_FOR_REFINEMENT ||
         GLOBAL_galerkin_state.shaftcullmode == ALWAYS_DO_SHAFTCULLING ) {
        freeCandidateList(globalCandidatesList);
    }

    globalCandidatesList = ocandlist;
}

/**
Link error estimation
*/

static double
hierarchicRefinementColorToError(COLOR rad) {
    RGB rgb;
    convertColorToRGB(rad, &rgb);
    return RGBMAXCOMPONENT(rgb);
}

/**
Instead of computing the approximation etc... error in radiance or power
error and comparing with a radiance resp. power threshold after weighting
with importance, we modify the threshold and always compare with the error
in radiance norm as if no importance is used. This enables us to skip
estimation of some error terms if it turns out that they are not necessary
anymore
*/
static double
hierarchicRefinementLinkErrorThreshold(INTERACTION *link, double rcv_area) {
    double threshold = 0.;

    switch ( GLOBAL_galerkin_state.error_norm ) {
        case RADIANCE_ERROR:
            threshold = hierarchicRefinementColorToError(GLOBAL_statistics_maxSelfEmittedRadiance) * GLOBAL_galerkin_state.rel_link_error_threshold;
            break;
        case POWER_ERROR:
            threshold = hierarchicRefinementColorToError(GLOBAL_statistics_maxSelfEmittedPower) * GLOBAL_galerkin_state.rel_link_error_threshold / (M_PI * rcv_area);
            break;
        default:
            logFatal(2, "hierarchicRefinementEvaluateInteraction", "Invalid error norm");
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
        threshold /= 2. * link->receiverElement->potential.f / GLOBAL_statistics_maxDirectPotential;
    }

    return threshold;
}

/**
Compute an estimate for the approximation error that would be made if the
candidate link were used for light transport. Use the sources un-shot
radiance when doing shooting and the total radiance when gathering
*/
static double
hierarchicRefinementApproximationError(INTERACTION *link, COLOR srcrho, COLOR rcvrho, double rcv_area) {
    COLOR error;
    COLOR srcrad;
    double approx_error = 0.0;
    double approx_error2;

    switch ( GLOBAL_galerkin_state.iteration_method ) {
        case GAUSS_SEIDEL:
        case JACOBI:
            if ( isCluster(link->sourceElement) && link->sourceElement != link->receiverElement ) {
                srcrad = maxClusterRadiance(link->sourceElement); /* sourceClusterRadiance(link); */
            } else {
                srcrad = link->sourceElement->radiance[0];
            }

            colorProductScaled(rcvrho, link->deltaK.f, srcrad, error);
            colorAbs(error, error);
            approx_error = hierarchicRefinementColorToError(error);
            break;

        case SOUTHWELL:
            if ( isCluster(link->sourceElement) && link->sourceElement != link->receiverElement ) {
                srcrad = sourceClusterRadiance(link); /* returns unshot radiance for shooting */
            } else {
                srcrad = link->sourceElement->unShotRadiance[0];
            }

            colorProductScaled(rcvrho, link->deltaK.f, srcrad, error);
            colorAbs(error, error);
            approx_error = hierarchicRefinementColorToError(error);

            if ( GLOBAL_galerkin_state.importance_driven && isCluster(link->receiverElement)) {
                /* make sure the link is also suited for transport of unshot potential
                 * from source to receiver. Note that it makes no sense to
                 * subdivide receiver patches (potential is only used to help
                 * choosing a radiance shooting patch. */
                approx_error2 = (hierarchicRefinementColorToError(srcrho) * link->deltaK.f * link->sourceElement->unShotPotential.f);

                /* compare potential error w.r.t. maximum direct potential or importance
                 * instead of selfemitted radiance or power. */
                switch ( GLOBAL_galerkin_state.error_norm ) {
                    case RADIANCE_ERROR:
                        approx_error2 *= hierarchicRefinementColorToError(GLOBAL_statistics_maxSelfEmittedRadiance) / GLOBAL_statistics_maxDirectPotential;
                        break;
                    case POWER_ERROR:
                        approx_error2 *=
                                hierarchicRefinementColorToError(GLOBAL_statistics_maxSelfEmittedPower) / M_PI / GLOBAL_statistics_maxDirectImportance;
                        break;
                }
                if ( approx_error2 > approx_error ) {
                    approx_error = approx_error2;
                }
            }
            break;

        default:
            logFatal(-1, "hierarchicRefinementApproximationError", "Invalid iteration method");
    }

    return approx_error;
}

/**
Estimates the error due to the variation of the source cluster radiance
as seen from a number of sample positions on the receiver element. Especially
when intra source cluster visibility is handled with a Z-buffer algorithm,
this operation is quite expensive and should be avoided when not stricktly
necessary
*/
static double
sourceClusterRadianceVariationError(INTERACTION *link, COLOR rcvrho, double rcv_area) {
    Vector3D rcverts[8];
    int i, nrcverts;
    COLOR minsrcrad;
    COLOR maxsrcrad;
    COLOR error;
    double K;

    K = (link->nsrc == 1 && link->nrcv == 1) ? link->K.f : link->K.p[0];
    if ( K == 0. || colorNull(rcvrho) || colorNull(link->sourceElement->radiance[0])) {
        /* receiver reflectivity or coupling coefficient or source radiance
         * is zero */
        return 0.;
    }

    nrcverts = galerkinElementVertices(link->receiverElement, rcverts);

    colorSetMonochrome(minsrcrad, HUGE);
    colorSetMonochrome(maxsrcrad, -HUGE);
    for ( i = 0; i < nrcverts; i++ ) {
        COLOR rad;
        rad = clusterRadianceToSamplePoint(link->sourceElement, rcverts[i]);
        colorMinimum(minsrcrad, rad, minsrcrad);
        colorMaximum(maxsrcrad, rad, maxsrcrad);
    }
    colorSubtract(maxsrcrad, minsrcrad, error);

    colorProductScaled(rcvrho, K / rcv_area, error, error);
    colorAbs(error, error);
    return hierarchicRefinementColorToError(error);
}

static INTERACTION_EVALUATION_CODE
hierarchicRefinementEvaluateInteraction(INTERACTION *link) {
    COLOR srcrho, rcvrho;
    double error, threshold, rcv_area, min_area;
    INTERACTION_EVALUATION_CODE code = ACCURATE_ENOUGH;

    if ( !GLOBAL_galerkin_state.hierarchical ) {
        return ACCURATE_ENOUGH;
    }    /* simply don't refine. */

    /* determine receiver area (projected visible area for a receiver cluster)
     * and reflectivity. */
    if ( isCluster(link->receiverElement)) {
        colorSetMonochrome(rcvrho, 1.);
        rcv_area = receiverClusterArea(link);
    } else {
        rcvrho = REFLECTIVITY(link->receiverElement->patch);
        rcv_area = link->receiverElement->area;
    }

    /* determine source reflectivity. */
    if ( isCluster(link->sourceElement)) {
        colorSetMonochrome(srcrho, 1.0f);
    } else
        srcrho = REFLECTIVITY(link->sourceElement->patch);

    /* determine error estimate and error threshold */
    threshold = hierarchicRefinementLinkErrorThreshold(link, rcv_area);
    error = hierarchicRefinementApproximationError(link, srcrho, rcvrho, rcv_area);

    if ( isCluster(link->sourceElement) && error < threshold && GLOBAL_galerkin_state.clustering_strategy != ISOTROPIC )
        error += sourceClusterRadianceVariationError(link, rcvrho, rcv_area);

    /* Minimal element area for which subdivision is allowed. */
    min_area = GLOBAL_statistics_totalArea * GLOBAL_galerkin_state.rel_min_elem_area;

    code = ACCURATE_ENOUGH;
    if ( error > threshold ) {
        /* A very simple but robust subdivision strategy: subdivide the
         * largest of the two elements in order to reduce the error. */
        if ((!(isCluster(link->sourceElement) && IsLightSource(link->sourceElement))) &&
            (rcv_area > link->sourceElement->area)) {
            if ( rcv_area > min_area ) {
                if ( isCluster(link->receiverElement))
                    code = SUBDIVIDE_RECEIVER_CLUSTER;
                else
                    code = REGULAR_SUBDIVIDE_RECEIVER;
            }
        } else {
            if ( isCluster(link->sourceElement))
                code = SUBDIVIDE_SOURCE_CLUSTER;
            else if ( link->sourceElement->area > min_area )
                code = REGULAR_SUBDIVIDE_SOURCE;
        }
    }

    return code;
}

/* ****************** Light transport computation ********************* */

/**
Computes light transport over the given interaction, which is supposed to be
accurate enough for doing so. Renormalisation and reflection and such is done
once for all accumulated received radiance during push-pull
*/
static void
hierarchicRefinementComputeLightTransport(INTERACTION *link) {
    COLOR *srcrad, *rcvrad, avsrclusrad;
    int alpha, beta, a, b;

    /* Update the number of effectively used radiance coefficients on the
     * receiver element. */
    a = MIN(link->nrcv, link->receiverElement->basisSize);
    b = MIN(link->nsrc, link->sourceElement->basisSize);
    if ( a > link->receiverElement->basisUsed ) {
        link->receiverElement->basisUsed = a;
    }
    if ( b > link->sourceElement->basisUsed ) {
        link->sourceElement->basisUsed = b;
    }

    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        srcrad = link->sourceElement->unShotRadiance;
    } else {
        srcrad = link->sourceElement->radiance;
    }

    if ( isCluster(link->sourceElement) && link->sourceElement != link->receiverElement ) {
        avsrclusrad = sourceClusterRadiance(link);
        srcrad = &avsrclusrad;
    }

    if ( isCluster(link->receiverElement) && link->sourceElement != link->receiverElement ) {
        clusterGatherRadiance(link, srcrad);
    } else {
        rcvrad = link->receiverElement->receivedRadiance;
        if ( link->nrcv == 1 && link->nsrc == 1 ) {
            colorAddScaled(rcvrad[0], link->K.f, srcrad[0], rcvrad[0]);
        } else {
            for ( alpha = 0; alpha < a; alpha++ ) {
                for ( beta = 0; beta < b; beta++ ) {
                    colorAddScaled(rcvrad[alpha], link->K.p[alpha * link->nsrc + beta],
                                   srcrad[beta], rcvrad[alpha]);
                }
            }
        }
    }

    if ( GLOBAL_galerkin_state.importance_driven ) {
        float K = ((link->nrcv == 1 && link->nsrc == 1) ? link->K.f : link->K.p[0]);
        COLOR rcvrho, srcrho;

        if ( GLOBAL_galerkin_state.iteration_method == GAUSS_SEIDEL ||
             GLOBAL_galerkin_state.iteration_method == JACOBI ) {
            if ( isCluster(link->receiverElement)) {
                colorSetMonochrome(rcvrho, 1.0f);
            } else {
                rcvrho = REFLECTIVITY(link->receiverElement->patch);
            }
            link->sourceElement->receivedPotential.f += K * hierarchicRefinementColorToError(rcvrho) * link->receiverElement->potential.f;
        } else if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
            if ( isCluster(link->sourceElement)) {
                colorSetMonochrome(srcrho, 1.0f);
            } else {
                srcrho = REFLECTIVITY(link->sourceElement->patch);
            }
            link->receiverElement->receivedPotential.f += K * hierarchicRefinementColorToError(srcrho) * link->sourceElement->unShotPotential.f;
        } else {
            logFatal(-1, "hierarchicRefinementComputeLightTransport", "Hela hola did you introduce a new iteration method or so??");
        }
    }
}

/**
Refinement procedures
*/

/**
Computes the form factor and error estimation coefficients. If the formfactor
is not zero, the data is filled in the INTERACTION pointed to by 'link'
and true is returned. If the elements don't interact, false is returned
*/
int
hierarchicRefinementCreateSubdivisionLink(GalerkinElement *rcv, GalerkinElement *src, INTERACTION *link) {
    link->receiverElement = rcv;
    link->sourceElement = src;

    // Always a constant approximation on cluster elements
    if ( isCluster(link->receiverElement)) {
        link->nrcv = 1;
    } else {
        link->nrcv = rcv->basisSize;
    }

    if ( isCluster(link->sourceElement)) {
        link->nsrc = 1;
    } else {
        link->nsrc = src->basisSize;
    }

    areaToAreaFormFactor(link, globalCandidatesList);

    return link->vis != 0;
}

/**
Duplicates the INTERACTION data and stores it with the receivers interactions
if doing gathering and with the source for shooting
*/
static void
hierarchicRefinementStoreInteraction(INTERACTION *link) {
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;

    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        src->interactions = InteractionListAdd(src->interactions, interactionDuplicate(link));
    } else {
        rcv->interactions = InteractionListAdd(rcv->interactions, interactionDuplicate(link));
    }
}

/**
Subdivides the source element, creates subinteractions and refines. If the
sub-interactions do not need to be refined any further, they are added to
either the sources, either the receivers interaction list, depending on the
iteration method being used. This routine always returns true indicating that
the passed interaction is always replaced by lower level interactions
*/
static int
hierarchicRefinementRegularSubdivideSource(INTERACTION *link) {
    GeometryListNode *ocandlist = hierarchicRefinementCull(link);
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;
    int i;

    galerkinElementRegularSubDivide(src);
    for ( i = 0; i < 4; i++ ) {
        GalerkinElement *child = src->regularSubElements[i];
        INTERACTION subinteraction;
        float ff[MAXBASISSIZE * MAXBASISSIZE];
        subinteraction.K.p = ff;    /* temporary storage for the formfactors */
        /* subinteraction.deltaK.p = */

        if ( hierarchicRefinementCreateSubdivisionLink(rcv, child, &subinteraction)) {
            if ( !refineRecursive(&subinteraction)) {
                hierarchicRefinementStoreInteraction(&subinteraction);
            }
        }
    }

    hierarchicRefinementUnCull(ocandlist);
    return true;
}

/**
Same, but subdivides the receiver element
*/
static int
hierarchicRefinementRegularSubdivideReceiver(INTERACTION *link) {
    GeometryListNode *ocandlist = hierarchicRefinementCull(link);
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;
    int i;

    galerkinElementRegularSubDivide(rcv);
    for ( i = 0; i < 4; i++ ) {
        INTERACTION subinteraction;
        float ff[MAXBASISSIZE * MAXBASISSIZE];
        GalerkinElement *child = rcv->regularSubElements[i];
        subinteraction.K.p = ff;

        if ( hierarchicRefinementCreateSubdivisionLink(child, src, &subinteraction)) {
            if ( !refineRecursive(&subinteraction)) {
                hierarchicRefinementStoreInteraction(&subinteraction);
            }
        }
    }

    hierarchicRefinementUnCull(ocandlist);
    return true;
}

/**
Replace the interaction by interactions with the subclusters of the source,
which is a cluster
*/
static int
hierarchicRefinementSubdivideSourceCluster(INTERACTION *link) {
    GeometryListNode *ocandlist = hierarchicRefinementCull(link);
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;
    ELEMENTLIST *subcluslist;

    for ( subcluslist = src->irregularSubElements; subcluslist; subcluslist = subcluslist->next ) {
        GalerkinElement *child = subcluslist->element;
        INTERACTION subinteraction;
        float ff[MAXBASISSIZE * MAXBASISSIZE];
        subinteraction.K.p = ff; // Temporary storage for the form-factors

        if ( !isCluster(child)) {
            Patch *the_patch = child->patch;
            if ((isCluster(rcv) &&
                 boundsBehindPlane(geomBounds(rcv->geom), &the_patch->normal, the_patch->planeConstant)) ||
                (!isCluster(rcv) && !facing(rcv->patch, the_patch))) {
                continue;
            }
        }

        if ( hierarchicRefinementCreateSubdivisionLink(rcv, child, &subinteraction)) {
            if ( !refineRecursive(&subinteraction)) {
                hierarchicRefinementStoreInteraction(&subinteraction);
            }
        }
    }

    hierarchicRefinementUnCull(ocandlist);
    return true;
}

/**
Replace the interaction by interactions with the subclusters of the receiver,
which is a cluster
*/
static int
hierarchicRefinementSubdivideReceiverCluster(INTERACTION *link) {
    GeometryListNode *ocandlist = hierarchicRefinementCull(link);
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;
    ELEMENTLIST *subcluslist;

    for ( subcluslist = rcv->irregularSubElements; subcluslist; subcluslist = subcluslist->next ) {
        GalerkinElement *child = subcluslist->element;
        INTERACTION subinteraction;
        float ff[MAXBASISSIZE * MAXBASISSIZE];
        subinteraction.K.p = ff;

        if ( !isCluster(child)) {
            Patch *the_patch = child->patch;
            if ((isCluster(src) &&
                 boundsBehindPlane(geomBounds(src->geom), &the_patch->normal, the_patch->planeConstant)) ||
                (!isCluster(src) && !facing(src->patch, the_patch))) {
                continue;
            }
        }

        if ( hierarchicRefinementCreateSubdivisionLink(child, src, &subinteraction)) {
            if ( !refineRecursive(&subinteraction)) {
                hierarchicRefinementStoreInteraction(&subinteraction);
            }
        }
    }

    hierarchicRefinementUnCull(ocandlist);
    return true;
}

/**
Recursievly refines the interaction. Returns true if the interaction was
effectively refined, so the specified interaction can be deleted. Returns false
if the interaction was not refined and is to be retained. If the interaction
does not need to be refined, light transport over the interaction is computed
*/
int
refineRecursive(INTERACTION *link) {
    int refined = false;

    switch ( hierarchicRefinementEvaluateInteraction(link)) {
        case ACCURATE_ENOUGH:
            hierarchicRefinementComputeLightTransport(link);
            refined = false;
            break;
        case REGULAR_SUBDIVIDE_SOURCE:
            refined = hierarchicRefinementRegularSubdivideSource(link);
            break;
        case REGULAR_SUBDIVIDE_RECEIVER:
            refined = hierarchicRefinementRegularSubdivideReceiver(link);
            break;
        case SUBDIVIDE_SOURCE_CLUSTER:
            refined = hierarchicRefinementSubdivideSourceCluster(link);
            break;
        case SUBDIVIDE_RECEIVER_CLUSTER:
            refined = hierarchicRefinementSubdivideReceiverCluster(link);
            break;
        default:
            logFatal(2, "refineRecursive", "Invalid result from hierarchicRefinementEvaluateInteraction()");
    }

    return refined;
}

void
refineInteraction(INTERACTION *link) {
    globalCandidatesList = GLOBAL_scene_clusteredWorld;    /* candidate occluder list for a pair of patches. */
    if ( GLOBAL_galerkin_state.exact_visibility && link->vis == 255 ) {
        globalCandidatesList = (GeometryListNode *) nullptr;
    }
    // we know for sure that there is full visibility

    if ( refineRecursive(link)) {
        if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL )
            link->sourceElement->interactions = InteractionListRemove(link->sourceElement->interactions, link);
        else
            link->receiverElement->interactions = InteractionListRemove(link->receiverElement->interactions, link);
        interactionDestroy(link);
    }
}

/**
Refines and computes light transport over all interactions of the given
toplevel element
*/
void
refineInteractions(GalerkinElement *top) {
    /* interactions will only be replaced by lower level interactions. We try refinement
     * beginning at the lowest levels in the hierarchy and working upwards to
     * prevent already refined interactions from being tested for refinement
     * again. */
    ITERATE_IRREGULAR_SUBELEMENTS(top, refineInteractions);
    ITERATE_REGULAR_SUB_ELEMENTS(top, refineInteractions);

    /* Iterate over the interactions. Interactions that are refined are removed from the
     * list in RefineInteraction(). ListIterate allows the current element to be deleted. */
    InteractionListIterate(top->interactions, refineInteraction);
}
