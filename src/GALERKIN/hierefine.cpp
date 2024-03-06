/**
Hierarchical refinement
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "scene/scene.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/formfactor.h"
#include "GALERKIN/shaftculling.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/hierefine.h"

/**
Shaft culling stuff for hierarchical refinement
*/

static bool refineRecursive(java::ArrayList<Geometry *> **candidatesList, Interaction *link, GalerkinState *state);

/**
Evaluates the interaction and returns a code telling whether it is accurate enough
for computing light transport, or what to do in order to reduce the
(estimated) error in the most efficient way. This is the famous oracle function
which is so crucial for efficient hierarchical refinement.

See DOC/galerkin.text
*/
enum INTERACTION_EVALUATION_CODE {
    ACCURATE_ENOUGH,
    REGULAR_SUBDIVIDE_SOURCE,
    REGULAR_SUBDIVIDE_RECEIVER,
    SUBDIVIDE_SOURCE_CLUSTER,
    SUBDIVIDE_RECEIVER_CLUSTER
};

/**
Does shaft-culling between elements in a link (if the user asked for it).
Updates the *candidatesList. Returns the old candidate list, so it can be restored
later (using hierarchicRefinementUnCull())
*/
static void
hierarchicRefinementCull(
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *state)
{
    if ( *candidatesList == nullptr ) {
        return;
    }

    if ( state->shaftCullMode == DO_SHAFT_CULLING_FOR_REFINEMENT ||
            state->shaftCullMode == ALWAYS_DO_SHAFT_CULLING ) {
        SHAFT shaft;
        SHAFT *the_shaft;

        if ( state->exact_visibility && !isCluster(link->receiverElement) && !isCluster(link->sourceElement)) {
            POLYGON rcvPolygon;
            POLYGON srcPolygon;
            the_shaft = constructPolygonToPolygonShaft(galerkinElementPolygon(link->receiverElement, &rcvPolygon),
                                                       galerkinElementPolygon(link->sourceElement, &srcPolygon),
                                                       &shaft);
        } else {
            BoundingBox srcBounds;
            BoundingBox rcvBounds;
            the_shaft = constructShaft(galerkinElementBounds(link->receiverElement, rcvBounds),
                                       galerkinElementBounds(link->sourceElement, srcBounds), &shaft);
        }
        if ( !the_shaft ) {
            logError("hierarchicRefinementCull", "Couldn't construct shaft");
            return;
        }

        if ( isCluster(link->receiverElement) ) {
            setShaftDontOpen(&shaft, link->receiverElement->geom);
        } else {
            setShaftOmit(&shaft, link->receiverElement->patch);
        }

        if ( isCluster(link->sourceElement) ) {
            setShaftDontOpen(&shaft, link->sourceElement->geom);
        } else {
            setShaftOmit(&shaft, link->sourceElement->patch);
        }

        if ( isClusteredGeometry ) {
            java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
            shaftCullGeometry(GLOBAL_scene_clusteredWorldGeom, &shaft, arr);
            *candidatesList = arr;
        } else {
            java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
            doShaftCulling(*candidatesList, &shaft, arr);
            *candidatesList = arr;
        }
    }
}

/**
Destroys the current geometryCandidatesList and restores the previous one (passed as
an argument)
*/
static void
hierarchicRefinementUnCull(
    java::ArrayList<Geometry *> **candidatesList,
    GalerkinState *state)
{
    if ( state->shaftCullMode == DO_SHAFT_CULLING_FOR_REFINEMENT ||
         state->shaftCullMode == ALWAYS_DO_SHAFT_CULLING ) {
        freeCandidateList(*candidatesList);
    }
}

/**
Link error estimation
*/

static double
hierarchicRefinementColorToError(COLOR rad) {
    RGB rgb{};
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
hierarchicRefinementLinkErrorThreshold(
    Interaction *link,
    double rcv_area,
    GalerkinState *state) {
    double threshold = 0.0;

    switch ( state->errorNorm ) {
        case RADIANCE_ERROR:
            threshold = hierarchicRefinementColorToError(GLOBAL_statistics.maxSelfEmittedRadiance) * state->relLinkErrorThreshold;
            break;
        case POWER_ERROR:
            threshold = hierarchicRefinementColorToError(GLOBAL_statistics.maxSelfEmittedPower) * state->relLinkErrorThreshold / (M_PI * rcv_area);
            break;
        default:
            logFatal(2, "hierarchicRefinementEvaluateInteraction", "Invalid error norm");
    }

    // Weight the error with the potential of the receiver in case of view-potential
    // driven gathering (potential is used in a totally different way in shooting
    // algorithms). It is assumed that the average direct potential is about half
    // of the maximum direct potential. This way, about an equal precision is
    // obtained in the visible parts of the scene if importance is used
    // instead of weighting the error, we weight the threshold with the inverse
    if ( state->importance_driven &&
         (state->iteration_method == JACOBI ||
          state->iteration_method == GAUSS_SEIDEL)) {
        threshold /= 2.0 * link->receiverElement->potential / GLOBAL_statistics_maxDirectPotential;
    }

    return threshold;
}

/**
Compute an estimate for the approximation error that would be made if the
candidate link were used for light transport. Use the sources un-shot
radiance when doing shooting and the total radiance when gathering
*/
static double
hierarchicRefinementApproximationError(
    Interaction *link,
    COLOR srcRho,
    COLOR rcvRho,
    GalerkinState *state)
{
    COLOR error;
    COLOR srcRad;
    double approxError = 0.0;
    double approxError2;

    switch ( state->iteration_method ) {
        case GAUSS_SEIDEL:
        case JACOBI:
            if ( isCluster(link->sourceElement) && link->sourceElement != link->receiverElement ) {
                srcRad = maxClusterRadiance(link->sourceElement);
            } else {
                srcRad = link->sourceElement->radiance[0];
            }

            colorProductScaled(rcvRho, link->deltaK.f, srcRad, error);
            colorAbs(error, error);
            approxError = hierarchicRefinementColorToError(error);
            break;

        case SOUTH_WELL:
            if ( isCluster(link->sourceElement) && link->sourceElement != link->receiverElement ) {
                srcRad = sourceClusterRadiance(link); // Returns un-shot radiance for shooting
            } else {
                srcRad = link->sourceElement->unShotRadiance[0];
            }

            colorProductScaled(rcvRho, link->deltaK.f, srcRad, error);
            colorAbs(error, error);
            approxError = hierarchicRefinementColorToError(error);

            if ( state->importance_driven && isCluster(link->receiverElement) ) {
                // Make sure the link is also suited for transport of un-shot potential
                // from source to receiver. Note that it makes no sense to
                // subdivide receiver patches (potential is only used to help
                // choosing a radiance shooting patch
                approxError2 = (hierarchicRefinementColorToError(srcRho) * link->deltaK.f * link->sourceElement->unShotPotential);

                // Compare potential error w.r.t. maximum direct potential or importance
                // instead of self-emitted radiance or power
                switch ( state->errorNorm ) {
                    case RADIANCE_ERROR:
                        approxError2 *= hierarchicRefinementColorToError(GLOBAL_statistics.maxSelfEmittedRadiance) / GLOBAL_statistics_maxDirectPotential;
                        break;
                    case POWER_ERROR:
                        approxError2 *=
                                hierarchicRefinementColorToError(GLOBAL_statistics.maxSelfEmittedPower) / M_PI / GLOBAL_statistics_maxDirectImportance;
                        break;
                }
                if ( approxError2 > approxError ) {
                    approxError = approxError2;
                }
            }
            break;

        default:
            logFatal(-1, "hierarchicRefinementApproximationError", "Invalid iteration method");
    }

    return approxError;
}

/**
Estimates the error due to the variation of the source cluster radiance
as seen from a number of sample positions on the receiver element. Especially
when intra source cluster visibility is handled with a Z-buffer algorithm,
this operation is quite expensive and should be avoided when not strictly
necessary
*/
static double
sourceClusterRadianceVariationError(Interaction *link, COLOR rcvRho, double rcv_area) {
    double K = (link->nsrc == 1 && link->nrcv == 1) ? link->K.f : link->K.p[0];
    if ( K == 0. || colorNull(rcvRho) || colorNull(link->sourceElement->radiance[0])) {
        // Receiver reflectivity or coupling coefficient or source radiance
        // is zero
        return 0.0;
    }

    Vector3D rcVertices[8];
    int numberOfRcVertices = galerkinElementVertices(link->receiverElement, rcVertices, 8);

    COLOR minimumSrcRad;
    COLOR maximumSrcRad;
    COLOR error;

    colorSetMonochrome(minimumSrcRad, HUGE);
    colorSetMonochrome(maximumSrcRad, -HUGE);
    for ( int i = 0; i < numberOfRcVertices; i++ ) {
        COLOR rad;
        rad = clusterRadianceToSamplePoint(link->sourceElement, rcVertices[i]);
        colorMinimum(minimumSrcRad, rad, minimumSrcRad);
        colorMaximum(maximumSrcRad, rad, maximumSrcRad);
    }
    colorSubtract(maximumSrcRad, minimumSrcRad, error);

    colorProductScaled(rcvRho, (float)(K / rcv_area), error, error);
    colorAbs(error, error);
    return hierarchicRefinementColorToError(error);
}

static INTERACTION_EVALUATION_CODE
hierarchicRefinementEvaluateInteraction(
    Interaction *link,
    GalerkinState *state)
{
    COLOR srcRho;
    COLOR rcvRho;
    double error;
    double threshold;
    double rcv_area;
    double min_area;
    INTERACTION_EVALUATION_CODE code;

    if ( !state->hierarchical ) {
        // Simply don't refine
        return ACCURATE_ENOUGH;
    }

    // Determine receiver area (projected visible area for a receiver cluster)
    // and reflectivity
    if ( isCluster(link->receiverElement) ) {
        colorSetMonochrome(rcvRho, 1.0);
        rcv_area = receiverClusterArea(link);
    } else {
        rcvRho = link->receiverElement->patch->radianceData->Rd;
        rcv_area = link->receiverElement->area;
    }

    // Determine source reflectivity
    if ( isCluster(link->sourceElement)) {
        colorSetMonochrome(srcRho, 1.0f);
    } else
        srcRho = link->sourceElement->patch->radianceData->Rd;

    // Determine error estimate and error threshold
    threshold = hierarchicRefinementLinkErrorThreshold(link, rcv_area, state);
    error = hierarchicRefinementApproximationError(link, srcRho, rcvRho, state);

    if ( isCluster(link->sourceElement) && error < threshold && state->clusteringStrategy != ISOTROPIC )
        error += sourceClusterRadianceVariationError(link, rcvRho, rcv_area);

    // Minimal element area for which subdivision is allowed
    min_area = GLOBAL_statistics.totalArea * state->relMinElemArea;

    code = ACCURATE_ENOUGH;
    if ( error > threshold ) {
        // A very simple but robust subdivision strategy: subdivide the
        // largest of the two elements in order to reduce the error
        if ( (!(isCluster(link->sourceElement) && (link->sourceElement->flags & IS_LIGHT_SOURCE)) ) &&
            (rcv_area > link->sourceElement->area) ) {
            if ( rcv_area > min_area ) {
                if ( isCluster(link->receiverElement) ) {
                    code = SUBDIVIDE_RECEIVER_CLUSTER;
                } else {
                    code = REGULAR_SUBDIVIDE_RECEIVER;
                }
            }
        } else {
            if ( isCluster(link->sourceElement) ) {
                code = SUBDIVIDE_SOURCE_CLUSTER;
            } else if ( link->sourceElement->area > min_area ) {
                code = REGULAR_SUBDIVIDE_SOURCE;
            }
        }
    }

    return code;
}

/** Light transport computation */

/**
Computes light transport over the given interaction, which is supposed to be
accurate enough for doing so. Renormalisation and reflection and such is done
once for all accumulated received radiance during push-pull
*/
static void
hierarchicRefinementComputeLightTransport(
    Interaction *link,
    GalerkinState *state)
{
    // Update the number of effectively used radiance coefficients on the
    // receiver element
    int a = intMin(link->nrcv, link->receiverElement->basisSize);
    int b = intMin(link->nsrc, link->sourceElement->basisSize);
    if ( a > link->receiverElement->basisUsed ) {
        link->receiverElement->basisUsed = (char)a;
    }
    if ( b > link->sourceElement->basisUsed ) {
        link->sourceElement->basisUsed = (char)b;
    }

    COLOR *srcRad;
    COLOR *rcvRad;
    if ( state->iteration_method == SOUTH_WELL ) {
        srcRad = link->sourceElement->unShotRadiance;
    } else {
        srcRad = link->sourceElement->radiance;
    }

    COLOR linkClusterRad;
    if ( isCluster(link->sourceElement) && link->sourceElement != link->receiverElement ) {
        linkClusterRad = sourceClusterRadiance(link);
        srcRad = &linkClusterRad;
    }

    if ( isCluster(link->receiverElement) && link->sourceElement != link->receiverElement ) {
        clusterGatherRadiance(link, srcRad);
    } else {
        rcvRad = link->receiverElement->receivedRadiance;
        if ( link->nrcv == 1 && link->nsrc == 1 ) {
            colorAddScaled(rcvRad[0], link->K.f, srcRad[0], rcvRad[0]);
        } else {
            for ( int alpha = 0; alpha < a; alpha++ ) {
                for ( int beta = 0; beta < b; beta++ ) {
                    colorAddScaled(
                     rcvRad[alpha],
                     link->K.p[alpha * link->nsrc + beta],
                     srcRad[beta],
                     rcvRad[alpha]);
                }
            }
        }
    }

    if ( state->importance_driven ) {
        float K = ((link->nrcv == 1 && link->nsrc == 1) ? link->K.f : link->K.p[0]);
        COLOR rcvRho;
        COLOR srcRho;

        if ( state->iteration_method == GAUSS_SEIDEL ||
             state->iteration_method == JACOBI ) {
            if ( isCluster(link->receiverElement) ) {
                colorSetMonochrome(rcvRho, 1.0f);
            } else {
                rcvRho = link->receiverElement->patch->radianceData->Rd;
            }
            link->sourceElement->receivedPotential += (float)(K * hierarchicRefinementColorToError(rcvRho) * link->receiverElement->potential);
        } else if ( state->iteration_method == SOUTH_WELL ) {
            if ( isCluster(link->sourceElement)) {
                colorSetMonochrome(srcRho, 1.0f);
            } else {
                srcRho = link->sourceElement->patch->radianceData->Rd;
            }
            link->receiverElement->receivedPotential += (float)(K * hierarchicRefinementColorToError(srcRho) * link->sourceElement->unShotPotential);
        } else {
            logFatal(-1, "hierarchicRefinementComputeLightTransport", "Did you introduce a new iteration method or so??");
        }
    }
}

/**
Refinement procedures
*/

/**
Computes the form factor and error estimation coefficients. If the form factor
is not zero, the data is filled in the INTERACTION pointed to by 'link'
and true is returned. If the elements don't interact, false is returned
*/
static int
hierarchicRefinementCreateSubdivisionLink(
    java::ArrayList<Geometry *> *candidatesList,
    GalerkinElement *rcv,
    GalerkinElement *src,
    Interaction *link)
{
    link->receiverElement = rcv;
    link->sourceElement = src;

    // Always a constant approximation on cluster elements
    if ( isCluster(link->receiverElement) ) {
        link->nrcv = 1;
    } else {
        link->nrcv = rcv->basisSize;
    }

    if ( isCluster(link->sourceElement) ) {
        link->nsrc = 1;
    } else {
        link->nsrc = src->basisSize;
    }

    bool isSceneGeometry = (candidatesList == GLOBAL_scene_geometries);
    bool isClusteredGeometry = (candidatesList == GLOBAL_scene_clusteredGeometries);
    areaToAreaFormFactor(link, candidatesList, isSceneGeometry, isClusteredGeometry);

    return link->vis != 0;
}

/**
Duplicates the INTERACTION data and stores it with the receivers interactions
if doing gathering and with the source for shooting
*/
static void
hierarchicRefinementStoreInteraction(Interaction *link, GalerkinState *state) {
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;

    if ( state->iteration_method == SOUTH_WELL ) {
        src->interactions = InteractionListAdd(src->interactions, interactionDuplicate(link));
    } else {
        rcv->interactions = InteractionListAdd(rcv->interactions, interactionDuplicate(link));
    }
}

/**
Subdivides the source element, creates sub-interactions and refines. If the
sub-interactions do not need to be refined any further, they are added to
either the sources, either the receivers interaction list, depending on the
iteration method being used. This routine always returns true indicating that
the passed interaction is always replaced by lower level interactions
*/
static void
hierarchicRefinementRegularSubdivideSource(
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *state)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(candidatesList, link, isClusteredGeometry, state);
    GalerkinElement *src = link->sourceElement;
    GalerkinElement *rcv = link->receiverElement;

    galerkinElementRegularSubDivide(src);
    for ( int i = 0; i < 4; i++ ) {
        GalerkinElement *child = src->regularSubElements[i];
        Interaction subInteraction{};
        float formFactors[MAX_BASIS_SIZE * MAX_BASIS_SIZE];
        subInteraction.K.p = formFactors; // Temporary storage for the form factors

        if ( hierarchicRefinementCreateSubdivisionLink(*candidatesList, rcv, child, &subInteraction) ) {
            if ( !refineRecursive(candidatesList, &subInteraction, state) ) {
                hierarchicRefinementStoreInteraction(&subInteraction, state);
            }
        }
    }

    hierarchicRefinementUnCull(candidatesList, state);
    *candidatesList = backup;
}

/**
Same, but subdivides the receiver element
*/
static void
hierarchicRefinementRegularSubdivideReceiver(
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *state)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(candidatesList, link, isClusteredGeometry, state);
    GalerkinElement *src = link->sourceElement;
    GalerkinElement *rcv = link->receiverElement;

    galerkinElementRegularSubDivide(rcv);
    for ( int i = 0; i < 4; i++ ) {
        Interaction subInteraction{};
        float ff[MAX_BASIS_SIZE * MAX_BASIS_SIZE];
        GalerkinElement *child = rcv->regularSubElements[i];
        subInteraction.K.p = ff;

        if ( hierarchicRefinementCreateSubdivisionLink(*candidatesList, child, src, &subInteraction) ) {
            if ( !refineRecursive(candidatesList, &subInteraction, state) ) {
                hierarchicRefinementStoreInteraction(&subInteraction, state);
            }
        }
    }

    hierarchicRefinementUnCull(candidatesList, state);
    *candidatesList = backup;
}

/**
Replace the interaction by interactions with the sub-clusters of the source,
which is a cluster
*/
static void
hierarchicRefinementSubdivideSourceCluster(
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *state)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(candidatesList, link, isClusteredGeometry, state);
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;

    for ( int i = 0; src->irregularSubElements != nullptr && i < src->irregularSubElements->size(); i++ ) {
        GalerkinElement *child = src->irregularSubElements->get(i);
        Interaction subInteraction{};
        float ff[MAX_BASIS_SIZE * MAX_BASIS_SIZE];
        subInteraction.K.p = ff; // Temporary storage for the form-factors

        if ( !isCluster(child)) {
            Patch *the_patch = child->patch;
            if ( (isCluster(rcv) &&
                 boundsBehindPlane(geomBounds(rcv->geom), &the_patch->normal, the_patch->planeConstant)) ||
                (!isCluster(rcv) && !facing(rcv->patch, the_patch)) ) {
                continue;
            }
        }

        if ( hierarchicRefinementCreateSubdivisionLink(*candidatesList, rcv, child, &subInteraction)) {
            if ( !refineRecursive(candidatesList, &subInteraction, state) ) {
                hierarchicRefinementStoreInteraction(&subInteraction, state);
            }
        }
    }

    hierarchicRefinementUnCull(candidatesList, state);
    *candidatesList = backup;
}

/**
Replace the interaction by interactions with the sub-clusters of the receiver,
which is a cluster
*/
static void
hierarchicRefinementSubdivideReceiverCluster(
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *state)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(candidatesList, link, isClusteredGeometry, state);
    GalerkinElement *src = link->sourceElement;
    GalerkinElement *rcv = link->receiverElement;

    for ( int i = 0; rcv->irregularSubElements != nullptr && i < rcv->irregularSubElements->size(); i++ ) {
        GalerkinElement *child = rcv->irregularSubElements->get(i);
        Interaction subInteraction{};
        float formFactor[MAX_BASIS_SIZE * MAX_BASIS_SIZE];
        subInteraction.K.p = formFactor;

        if ( !isCluster(child) ) {
            Patch *the_patch = child->patch;
            if ( (isCluster(src) &&
                 boundsBehindPlane(geomBounds(src->geom), &the_patch->normal, the_patch->planeConstant)) ||
                (!isCluster(src) && !facing(src->patch, the_patch)) ) {
                continue;
            }
        }

        if ( hierarchicRefinementCreateSubdivisionLink(*candidatesList, child, src, &subInteraction)) {
            if ( !refineRecursive(candidatesList, &subInteraction, state) ) {
                hierarchicRefinementStoreInteraction(&subInteraction, state);
            }
        }
    }

    hierarchicRefinementUnCull(candidatesList, state);
    *candidatesList = backup;
}

/**
Recursively refines the interaction. Returns true if the interaction was
effectively refined, so the specified interaction can be deleted. Returns false
if the interaction was not refined and is to be retained. If the interaction
does not need to be refined, light transport over the interaction is computed
*/
static bool
refineRecursive(
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    GalerkinState *state)
{
    bool refined = false;

    bool isClusteredGeometry = (*candidatesList == GLOBAL_scene_clusteredGeometries);
    switch ( hierarchicRefinementEvaluateInteraction(link, state) ) {
        case ACCURATE_ENOUGH:
            hierarchicRefinementComputeLightTransport(link, state);
            refined = false;
            break;
        case REGULAR_SUBDIVIDE_SOURCE:
            hierarchicRefinementRegularSubdivideSource(candidatesList, link, isClusteredGeometry, state);
            refined = true;
            break;
        case REGULAR_SUBDIVIDE_RECEIVER:
            hierarchicRefinementRegularSubdivideReceiver(candidatesList, link, isClusteredGeometry, state);
            refined = true;
            break;
        case SUBDIVIDE_SOURCE_CLUSTER:
            hierarchicRefinementSubdivideSourceCluster(candidatesList, link, isClusteredGeometry, state);
            refined = true;
            break;
        case SUBDIVIDE_RECEIVER_CLUSTER:
            hierarchicRefinementSubdivideReceiverCluster(candidatesList, link, isClusteredGeometry, state);
            refined = true;
            break;
        default:
            logFatal(2, "refineRecursive", "Invalid result from hierarchicRefinementEvaluateInteraction()");
    }

    return refined;
}

/**
Candidate occluder list for a pair of patches, note it is changed inside the methods!
*/
static bool
refineInteraction(Interaction *link, GalerkinState *state) {
    java::ArrayList<Geometry *> *candidateOccluderList = GLOBAL_scene_clusteredGeometries;

    if ( state->exact_visibility && link->vis == 255 ) {
        candidateOccluderList = nullptr;
    }

    // We know for sure that there is full visibility
    return refineRecursive(&candidateOccluderList, link, state);
}

static void
removeRefinedInteractions(const GalerkinState *state, java::ArrayList<Interaction *> *interactionsToRemove) {
    for ( int i = 0; i < interactionsToRemove->size(); i++ ) {
        Interaction *interaction = interactionsToRemove->get(i);
        if ( state->iteration_method == SOUTH_WELL ) {
            interaction->sourceElement->interactions = InteractionListRemove(interaction->sourceElement->interactions, interaction);
        } else {
            interaction->receiverElement->interactions = InteractionListRemove(interaction->receiverElement->interactions, interaction);
        }
        interactionDestroy(interaction);
    }
}

/**
Refines and computes light transport over all interactions of the given
toplevel element
*/
void
refineInteractions(GalerkinElement *parentElement, GalerkinState *state) {
    // Interactions will only be replaced by lower level interactions. We try refinement
    // beginning at the lowest levels in the hierarchy and working upwards to
    // prevent already refined interactions from being tested for refinement
    // again
    for ( int i = 0; parentElement->irregularSubElements != nullptr && i < parentElement->irregularSubElements->size(); i++ ) {
        refineInteractions(parentElement->irregularSubElements->get(i), state);
    }

    if ( parentElement->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            refineInteractions(parentElement->regularSubElements[i], state);
        }
    }

    // Iterate over the interactions. Interactions that are refined are removed from the list
    java::ArrayList<Interaction *> *interactionsToRemove = new java::ArrayList<Interaction *>();

    for ( InteractionListNode *window = parentElement->interactions; window != nullptr; window = window->next ) {
        Interaction *interaction = window->interaction;
        if ( refineInteraction(interaction, state) ) {
            interactionsToRemove->add(interaction);
        }
    }
    removeRefinedInteractions(state, interactionsToRemove);
    delete interactionsToRemove;
}
