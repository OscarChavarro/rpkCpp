/**
Hierarchical refinement
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/mymath.h"
#include "material/statistics.h"
#include "GALERKIN/basisgalerkin.h"
#include "FormFactorStrategy.h"
#include "GALERKIN/Shaft.h"
#include "GALERKIN/Interaction.h"
#include "GALERKIN/processing/ClusterTraversalStrategy.h"
#include "GALERKIN/processing/HierarchicalRefinementStrategy.h"

/**
Does shaft-culling between elements in a link (if the user asked for it).
Updates the *candidatesList. Returns the old candidate list, so it can be restored
later (using hierarchicRefinementUnCull())
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementCull(
    Scene *scene,
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
        Shaft shaft;

        if ( state->exactVisibility && !link->receiverElement->isCluster() && !link->sourceElement->isCluster() ) {
            Polygon rcvPolygon;
            Polygon srcPolygon;
            link->receiverElement->initPolygon(&rcvPolygon);
            link->sourceElement->initPolygon(&srcPolygon);
            shaft.constructFromPolygonToPolygon(&rcvPolygon, &srcPolygon);
        } else {
            BoundingBox srcBounds;
            BoundingBox rcvBounds;
            shaft.constructFromBoundingBoxes(
                link->receiverElement->bounds(&rcvBounds),
                link->sourceElement->bounds(&srcBounds));
        }

        if ( link->receiverElement->isCluster() ) {
            shaft.setShaftDontOpen(link->receiverElement->geometry);
        } else {
            shaft.setShaftOmit(link->receiverElement->patch);
        }

        if ( link->sourceElement->isCluster() ) {
            shaft.setShaftDontOpen( link->sourceElement->geometry);
        } else {
            shaft.setShaftOmit(link->sourceElement->patch);
        }

        if ( isClusteredGeometry ) {
            java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
            shaft.cullGeometry(scene->clusteredRootGeometry, arr);
            *candidatesList = arr;
        } else {
            java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
            shaft.doCulling(*candidatesList, arr);
            *candidatesList = arr;
        }
    }
}

/**
Destroys the current geometryCandidatesList and restores the previous one (passed as
an argument)
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementUnCull(
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

double
HierarchicalRefinementStrategy::hierarchicRefinementColorToError(ColorRgb rad) {
    return rad.maximumComponent();
}

/**
Instead of computing the approximation etc... error in radiance or power
error and comparing with a radiance resp. power threshold after weighting
with importance, we modify the threshold and always compare with the error
in radiance norm as if no importance is used. This enables us to skip
estimation of some error terms if it turns out that they are not necessary
anymore
*/
double
HierarchicalRefinementStrategy::hierarchicRefinementLinkErrorThreshold(
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
    if ( state->importanceDriven &&
         (state->galerkinIterationMethod == JACOBI ||
          state->galerkinIterationMethod == GAUSS_SEIDEL) ) {
        threshold /= 2.0 * link->receiverElement->potential / GLOBAL_statistics.maxDirectPotential;
    }

    return threshold;
}

/**
Compute an estimate for the approximation error that would be made if the
candidate link were used for light transport. Use the sources un-shot
radiance when doing shooting and the total radiance when gathering
*/
double
HierarchicalRefinementStrategy::hierarchicRefinementApproximationError(
    Interaction *link,
    ColorRgb srcRho,
    ColorRgb rcvRho,
    GalerkinState *galerkinState)
{
    ColorRgb error;
    ColorRgb srcRad;
    double approxError = 0.0;
    double approxError2;

    switch ( galerkinState->galerkinIterationMethod ) {
        case GAUSS_SEIDEL:
        case JACOBI:
            if ( link->sourceElement->isCluster() && link->sourceElement != link->receiverElement ) {
                srcRad = ClusterTraversalStrategy::maxRadiance(link->sourceElement, galerkinState);
            } else {
                srcRad = link->sourceElement->radiance[0];
            }

            error.scalarProductScaled(rcvRho, link->deltaK[0], srcRad);
            error.abs();
            approxError = hierarchicRefinementColorToError(error);
            break;

        case SOUTH_WELL:
            if ( link->sourceElement->isCluster() && link->sourceElement != link->receiverElement ) {
                srcRad = ClusterTraversalStrategy::sourceClusterRadiance(link, galerkinState); // Returns un-shot radiance for shooting
            } else {
                srcRad = link->sourceElement->unShotRadiance[0];
            }

            error.scalarProductScaled(rcvRho, link->deltaK[0], srcRad);
            error.abs();
            approxError = hierarchicRefinementColorToError(error);

            if ( galerkinState->importanceDriven && link->receiverElement->isCluster() ) {
                // Make sure the link is also suited for transport of un-shot potential
                // from source to receiver. Note that it makes no sense to
                // subdivide receiver patches (potential is only used to help
                // choosing a radiance shooting patch
                approxError2 = (hierarchicRefinementColorToError(srcRho) * link->deltaK[0] * link->sourceElement->unShotPotential);

                // Compare potential error w.r.t. maximum direct potential or importance
                // instead of self-emitted radiance or power
                switch ( galerkinState->errorNorm ) {
                    case RADIANCE_ERROR:
                        approxError2 *= hierarchicRefinementColorToError(GLOBAL_statistics.maxSelfEmittedRadiance) / GLOBAL_statistics.maxDirectPotential;
                        break;
                    case POWER_ERROR:
                        approxError2 *=
                                hierarchicRefinementColorToError(GLOBAL_statistics.maxSelfEmittedPower) / M_PI / GLOBAL_statistics.maxDirectImportance;
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
double
HierarchicalRefinementStrategy::sourceClusterRadianceVariationError(Interaction *link, ColorRgb rcvRho, double rcv_area, GalerkinState *galerkinState) {
    double K = link->K[0];
    if ( K == 0.0 || rcvRho.isBlack() || link->sourceElement->radiance[0].isBlack() ) {
        // Receiver reflectivity or coupling coefficient or source radiance
        // is zero
        return 0.0;
    }

    Vector3D rcVertices[8];
    int numberOfRcVertices = link->receiverElement->vertices(rcVertices, 8);

    ColorRgb minimumSrcRad;
    ColorRgb maximumSrcRad;
    ColorRgb error;

    minimumSrcRad.setMonochrome(HUGE);
    maximumSrcRad.setMonochrome(-HUGE);
    for ( int i = 0; i < numberOfRcVertices; i++ ) {
        ColorRgb rad;
        rad = ClusterTraversalStrategy::clusterRadianceToSamplePoint(link->sourceElement, rcVertices[i], galerkinState);
        minimumSrcRad.minimum(minimumSrcRad, rad);
        maximumSrcRad.maximum(maximumSrcRad, rad);
    }
    error.subtract(maximumSrcRad, minimumSrcRad);

    error.scalarProductScaled(rcvRho, (float) (K / rcv_area), error);
    error.abs();
    return hierarchicRefinementColorToError(error);
}

INTERACTION_EVALUATION_CODE
HierarchicalRefinementStrategy::hierarchicRefinementEvaluateInteraction(
    Interaction *link,
    GalerkinState *galerkinState)
{
    ColorRgb srcRho;
    ColorRgb rcvRho;
    double error;
    double threshold;
    double receiveArea;
    double minimumArea;
    INTERACTION_EVALUATION_CODE code;

    if ( !galerkinState->hierarchical ) {
        // Simply don't refine
        return ACCURATE_ENOUGH;
    }

    // Determine receiver area (projected visible area for a receiver cluster)
    // and reflectivity
    if ( link->receiverElement->isCluster() ) {
        rcvRho.setMonochrome(1.0);
        receiveArea = ClusterTraversalStrategy::receiverArea(link, galerkinState);
    } else {
        rcvRho = link->receiverElement->patch->radianceData->Rd;
        receiveArea = link->receiverElement->area;
    }

    // Determine source reflectivity
    if ( link->sourceElement->isCluster() ) {
        srcRho.setMonochrome(1.0f);
    } else
        srcRho = link->sourceElement->patch->radianceData->Rd;

    // Determine error estimate and error threshold
    threshold = hierarchicRefinementLinkErrorThreshold(link, receiveArea, galerkinState);
    error = hierarchicRefinementApproximationError(link, srcRho, rcvRho, galerkinState);

    if ( link->sourceElement->isCluster() && error < threshold && galerkinState->clusteringStrategy != ISOTROPIC )
        error += sourceClusterRadianceVariationError(link, rcvRho, receiveArea, galerkinState);

    // Minimal element area for which subdivision is allowed
    minimumArea = GLOBAL_statistics.totalArea * galerkinState->relMinElemArea;

    code = ACCURATE_ENOUGH;
    if ( error > threshold ) {
        // A very simple but robust subdivision strategy: subdivide the
        // largest of the two elements in order to reduce the error
        if ((!(link->sourceElement->isCluster() && (link->sourceElement->flags & IS_LIGHT_SOURCE_MASK)) ) &&
            (receiveArea > link->sourceElement->area) ) {
            if ( receiveArea > minimumArea ) {
                if ( link->receiverElement->isCluster() ) {
                    code = SUBDIVIDE_RECEIVER_CLUSTER;
                } else {
                    code = REGULAR_SUBDIVIDE_RECEIVER;
                }
            }
        } else {
            if ( link->sourceElement->isCluster() ) {
                code = SUBDIVIDE_SOURCE_CLUSTER;
            } else if ( link->sourceElement->area > minimumArea ) {
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
void
HierarchicalRefinementStrategy::hierarchicRefinementComputeLightTransport(
    Interaction *link,
    GalerkinState *galerkinState)
{
    // Update the number of effectively used radiance coefficients on the
    // receiver element
    int a = intMin(link->numberOfBasisFunctionsOnReceiver, link->receiverElement->basisSize);
    int b = intMin(link->numberOfBasisFunctionsOnSource, link->sourceElement->basisSize);
    if ( a > link->receiverElement->basisUsed ) {
        link->receiverElement->basisUsed = (char)a;
    }
    if ( b > link->sourceElement->basisUsed ) {
        link->sourceElement->basisUsed = (char)b;
    }

    ColorRgb *srcRad;
    ColorRgb *rcvRad;
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        srcRad = link->sourceElement->unShotRadiance;
    } else {
        srcRad = link->sourceElement->radiance;
    }

    ColorRgb linkClusterRad;
    if ( link->sourceElement->isCluster() && link->sourceElement != link->receiverElement ) {
        linkClusterRad = ClusterTraversalStrategy::sourceClusterRadiance(link, galerkinState);
        srcRad = &linkClusterRad;
    }

    if ( link->receiverElement->isCluster() && link->sourceElement != link->receiverElement ) {
        ClusterTraversalStrategy::gatherRadiance(link, srcRad, galerkinState);
    } else {
        rcvRad = link->receiverElement->receivedRadiance;
        if ( link->numberOfBasisFunctionsOnReceiver == 1 && link->numberOfBasisFunctionsOnSource == 1 ) {
            rcvRad[0].addScaled(rcvRad[0], link->K[0], srcRad[0]);
        } else {
            for ( int alpha = 0; alpha < a; alpha++ ) {
                for ( int beta = 0; beta < b; beta++ ) {
                    rcvRad[alpha].addScaled(
                        rcvRad[alpha],
                        link->K[alpha * link->numberOfBasisFunctionsOnSource + beta],
                        srcRad[beta]);
                }
            }
        }
    }

    if ( galerkinState->importanceDriven ) {
        float K = link->K[0];
        ColorRgb rcvRho;
        ColorRgb srcRho;

        if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ||
             galerkinState->galerkinIterationMethod == JACOBI ) {
            if ( link->receiverElement->isCluster() ) {
                rcvRho.setMonochrome(1.0f);
            } else {
                rcvRho = link->receiverElement->patch->radianceData->Rd;
            }
            link->sourceElement->receivedPotential += (float)(K * hierarchicRefinementColorToError(rcvRho) * link->receiverElement->potential);
        } else if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
            if ( link->sourceElement->isCluster() ) {
                srcRho.setMonochrome(1.0f);
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
int
HierarchicalRefinementStrategy::hierarchicRefinementCreateSubdivisionLink(
    Scene *scene,
    java::ArrayList<Geometry *> *candidatesList,
    GalerkinElement *rcv,
    GalerkinElement *src,
    Interaction *link,
    GalerkinState *galerkinState)
{
    link->receiverElement = rcv;
    link->sourceElement = src;

    // Always a constant approximation on cluster elements
    if ( link->receiverElement->isCluster() ) {
        link->numberOfBasisFunctionsOnReceiver = 1;
    } else {
        link->numberOfBasisFunctionsOnReceiver = rcv->basisSize;
    }

    if ( link->sourceElement->isCluster() ) {
        link->numberOfBasisFunctionsOnSource = 1;
    } else {
        link->numberOfBasisFunctionsOnSource = src->basisSize;
    }

    const bool isSceneGeometry = (candidatesList == scene->geometryList);
    const bool isClusteredGeometry = (candidatesList == scene->clusteredGeometryList);
    FormFactorStrategy::computeAreaToAreaFormFactorVisibility(
        (const VoxelGrid *)scene->voxelGrid,
        candidatesList,
        isSceneGeometry,
        isClusteredGeometry,
        link,
        galerkinState);

    return link->visibility != 0;
}

/**
Duplicates the INTERACTION data and stores it with the receivers interactions
if doing gathering and with the source for shooting
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementStoreInteraction(Interaction *link, GalerkinState *state) {
    Interaction *newLink = Interaction::interactionDuplicate(link);

    if ( state->galerkinIterationMethod == SOUTH_WELL ) {
        link->sourceElement->interactions->add(newLink);
    } else {
        link->receiverElement->interactions->add(newLink);
    }
}

/**
Subdivides the source element, creates sub-interactions and refines. If the
sub-interactions do not need to be refined any further, they are added to
either the sources, either the receivers interaction list, depending on the
iteration method being used. This routine always replaces interaction with
lower level sub-interactions
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementRegularSubdivideSource(
    Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *galerkinState,
    RenderOptions *renderOptions)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, link, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = link->sourceElement;
    GalerkinElement *receiverElement = link->receiverElement;

    sourceElement->regularSubDivide(renderOptions);
    for ( int i = 0; i < 4; i++ ) {
        GalerkinElement *child = (GalerkinElement *)sourceElement->regularSubElements[i];
        Interaction subInteraction{};
        subInteraction.K = new float[MAX_BASIS_SIZE * MAX_BASIS_SIZE];

        if ( hierarchicRefinementCreateSubdivisionLink(
                scene,
                *candidatesList,
                receiverElement,
                child,
                &subInteraction,
                galerkinState) ) {
            if ( !refineRecursive(
                    scene,
                    candidatesList,
                    &subInteraction,
                    galerkinState,
                    renderOptions) ) {
                hierarchicRefinementStoreInteraction(&subInteraction, galerkinState);
            }
        }
    }

    hierarchicRefinementUnCull(candidatesList, galerkinState);
    *candidatesList = backup;
}

/**
Same, but subdivides the receiver element
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementRegularSubdivideReceiver(
    Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *galerkinState,
    RenderOptions *renderOptions)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, link, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = link->sourceElement;
    GalerkinElement *receiverElement = link->receiverElement;

    receiverElement->regularSubDivide(renderOptions);
    for ( int i = 0; i < 4; i++ ) {
        Interaction subInteraction{};
        GalerkinElement *child = (GalerkinElement *)receiverElement->regularSubElements[i];
        subInteraction.K = new float[MAX_BASIS_SIZE * MAX_BASIS_SIZE];

        if ( hierarchicRefinementCreateSubdivisionLink(
                scene,
                *candidatesList,
                child, sourceElement,
                &subInteraction,
                galerkinState) ) {
            if ( !refineRecursive(
                    scene,
                    candidatesList,
                    &subInteraction,
                    galerkinState,
                    renderOptions) ) {
                hierarchicRefinementStoreInteraction(&subInteraction, galerkinState);
            }
        }
    }

    hierarchicRefinementUnCull(candidatesList, galerkinState);
    *candidatesList = backup;
}

/**
Replace the interaction by interactions with the sub-clusters of the source,
which is a cluster
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementSubdivideSourceCluster(
    Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *galerkinState,
    RenderOptions *renderOptions)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, link, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = link->sourceElement;
    GalerkinElement *receiverElement = link->receiverElement;

    for ( int i = 0; sourceElement->irregularSubElements != nullptr && i < sourceElement->irregularSubElements->size(); i++ ) {
        GalerkinElement *childElement = (GalerkinElement *)sourceElement->irregularSubElements->get(i);
        Interaction subInteraction{};
        subInteraction.K = new float[MAX_BASIS_SIZE * MAX_BASIS_SIZE];

        if ( !childElement->isCluster() ) {
            Patch *thePatch = childElement->patch;
            if ( (receiverElement->isCluster() && getBoundingBox(receiverElement->geometry).behindPlane(&thePatch->normal, thePatch->planeConstant)) ||
                (!receiverElement->isCluster() && !receiverElement->patch->facing(thePatch)) ) {
                continue;
            }
        }

        if ( hierarchicRefinementCreateSubdivisionLink(
                scene,
                *candidatesList,
                receiverElement,
                childElement,
                &subInteraction,
                galerkinState) ) {
            if ( !refineRecursive(
                    scene,
                    candidatesList,
                    &subInteraction,
                    galerkinState,
                    renderOptions) ) {
                hierarchicRefinementStoreInteraction(&subInteraction, galerkinState);
            }
        }
    }

    hierarchicRefinementUnCull(candidatesList, galerkinState);
    *candidatesList = backup;
}

/**
Replace the interaction by interactions with the sub-clusters of the receiver,
which is a cluster
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementSubdivideReceiverCluster(
    Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *galerkinState,
    RenderOptions *renderOptions)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, link, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = link->sourceElement;
    GalerkinElement *receiverElement = link->receiverElement;

    for ( int i = 0; receiverElement->irregularSubElements != nullptr && i < receiverElement->irregularSubElements->size(); i++ ) {
        GalerkinElement *child = (GalerkinElement *)receiverElement->irregularSubElements->get(i);
        Interaction subInteraction{};
        subInteraction.K = new float [MAX_BASIS_SIZE * MAX_BASIS_SIZE];

        if ( !child->isCluster() ) {
            Patch *thePatch = child->patch;
            if ( (sourceElement->isCluster() && getBoundingBox(sourceElement->geometry).behindPlane(&thePatch->normal, thePatch->planeConstant)) ||
                (!sourceElement->isCluster() && !sourceElement->patch->facing(thePatch)) ) {
                continue;
            }
        }

        if ( hierarchicRefinementCreateSubdivisionLink(
                scene,
                *candidatesList,
                child,
                sourceElement,
                &subInteraction,
                galerkinState) ) {
            if ( !refineRecursive(
                    scene,
                    candidatesList,
                    &subInteraction,
                    galerkinState,
                    renderOptions) ) {
                hierarchicRefinementStoreInteraction(&subInteraction, galerkinState);
            }
        }
    }

    hierarchicRefinementUnCull(candidatesList, galerkinState);
    *candidatesList = backup;
}

/**
Recursively refines the interaction. Returns true if the interaction was
effectively refined, so the specified interaction can be deleted. Returns false
if the interaction was not refined and is to be retained. If the interaction
does not need to be refined, light transport over the interaction is computed
*/
bool
HierarchicalRefinementStrategy::refineRecursive(
    Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    GalerkinState *state,
    RenderOptions *renderOptions)
{
    bool refined = false;

    bool isClusteredGeometry = (*candidatesList == scene->clusteredGeometryList);
    switch ( hierarchicRefinementEvaluateInteraction(link, state) ) {
        case ACCURATE_ENOUGH:
            hierarchicRefinementComputeLightTransport(link, state);
            refined = false;
            break;
        case REGULAR_SUBDIVIDE_SOURCE:
            hierarchicRefinementRegularSubdivideSource(
                scene,
                candidatesList,
                link,
                isClusteredGeometry,
                state,
                renderOptions);
            refined = true;
            break;
        case REGULAR_SUBDIVIDE_RECEIVER:
            hierarchicRefinementRegularSubdivideReceiver(
                scene,
                candidatesList,
                link,
                isClusteredGeometry,
                state,
                renderOptions);
            refined = true;
            break;
        case SUBDIVIDE_SOURCE_CLUSTER:
            hierarchicRefinementSubdivideSourceCluster(
                scene,
                candidatesList,
                link,
                isClusteredGeometry,
                state,
                renderOptions);
            refined = true;
            break;
        case SUBDIVIDE_RECEIVER_CLUSTER:
            hierarchicRefinementSubdivideReceiverCluster(
                scene,
                candidatesList,
                link,
                isClusteredGeometry,
                state,
                renderOptions);
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
bool
HierarchicalRefinementStrategy::HierarchicalRefinementStrategy::refineInteraction(Scene *scene, Interaction *link, GalerkinState *state, RenderOptions *renderOptions) {
    java::ArrayList<Geometry *> *candidateOccluderList = scene->clusteredGeometryList;

    if ( state->exactVisibility && link->visibility == 255 ) {
        candidateOccluderList = nullptr;
    }

    // We know for sure that there is full visibility
    return refineRecursive(scene, &candidateOccluderList, link, state, renderOptions);
}

void
HierarchicalRefinementStrategy::removeRefinedInteractions(const GalerkinState *state, java::ArrayList<Interaction *> *interactionsToRemove) {
    for ( int i = 0; i < interactionsToRemove->size(); i++ ) {
        Interaction *interaction = interactionsToRemove->get(i);
        if ( state->galerkinIterationMethod == SOUTH_WELL ) {
            interaction->sourceElement->interactions->remove(interaction);
        } else {
            interaction->receiverElement->interactions->remove(interaction);
        }
        Interaction::interactionDestroy(interaction);
    }
}

/**
Refines and computes light transport over all interactions of the given
toplevel element
*/
void
HierarchicalRefinementStrategy::refineInteractions(
    Scene *scene,
    GalerkinElement *parentElement,
    GalerkinState *state,
    RenderOptions *renderOptions)
{
    // Interactions will only be replaced by lower level interactions. We try refinement
    // beginning at the lowest levels in the hierarchy and working upwards to
    // prevent already refined interactions from being tested for refinement
    // again
    for ( int i = 0;
          parentElement->irregularSubElements != nullptr && i < parentElement->irregularSubElements->size();
          i++ ) {
        refineInteractions(
            scene,
            (GalerkinElement *)parentElement->irregularSubElements->get(i),
            state,
            renderOptions);
    }

    if ( parentElement->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            refineInteractions(
                scene,
                (GalerkinElement *)parentElement->regularSubElements[i],
                state,
                renderOptions);
        }
    }

    // Iterate over the interactions. Interactions that are refined are removed from the list
    java::ArrayList<Interaction *> *interactionsToRemove = new java::ArrayList<Interaction *>();

    for ( int i = 0; parentElement->interactions != nullptr && i < parentElement->interactions->size(); i++ ) {
        Interaction *interaction = parentElement->interactions->get(i);
        if ( refineInteraction(
                scene,
                interaction,
                state,
                renderOptions) ) {
            interactionsToRemove->add(interaction);
        }
    }
    removeRefinedInteractions(state, interactionsToRemove);
    delete interactionsToRemove;
}
