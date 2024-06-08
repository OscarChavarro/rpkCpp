/**
Hierarchical refinement
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/Statistics.h"
#include "GALERKIN/processing/FormFactorStrategy.h"
#include "GALERKIN/Shaft.h"
#include "GALERKIN/processing/ClusterTraversalStrategy.h"
#include "GALERKIN/processing/HierarchicalRefinementStrategy.h"

/**
Does shaft-culling between elements in a interaction (if the user asked for it).
Updates the *candidatesList. Returns the old candidate list, so it can be restored
later (using hierarchicRefinementUnCull())
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementCull(
    const Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *interaction,
    bool isClusteredGeometry,
    const GalerkinState *galerkinState)
{
    if ( *candidatesList == nullptr ) {
        return;
    }

    if ( galerkinState->shaftCullMode == GalerkinShaftCullMode::DO_SHAFT_CULLING_FOR_REFINEMENT ||
         galerkinState->shaftCullMode == GalerkinShaftCullMode::ALWAYS_DO_SHAFT_CULLING ) {
        Shaft shaft;

        if ( galerkinState->exactVisibility
          && !interaction->receiverElement->isCluster()
          && !interaction->sourceElement->isCluster() ) {
            Polygon rcvPolygon;
            Polygon srcPolygon;
            interaction->receiverElement->initPolygon(&rcvPolygon);
            interaction->sourceElement->initPolygon(&srcPolygon);
            shaft.constructFromPolygonToPolygon(&rcvPolygon, &srcPolygon);
        } else {
            BoundingBox srcBounds;
            BoundingBox rcvBounds;
            shaft.constructFromBoundingBoxes(
                    interaction->receiverElement->bounds(&rcvBounds),
                    interaction->sourceElement->bounds(&srcBounds));
        }

        if ( interaction->receiverElement->isCluster() ) {
            shaft.setShaftDontOpen(interaction->receiverElement->geometry);
        } else {
            shaft.setShaftOmit(interaction->receiverElement->patch);
        }

        if ( interaction->sourceElement->isCluster() ) {
            shaft.setShaftDontOpen(interaction->sourceElement->geometry);
        } else {
            shaft.setShaftOmit(interaction->sourceElement->patch);
        }

        if ( isClusteredGeometry ) {
            java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
            shaft.cullGeometry(scene->clusteredRootGeometry, arr, galerkinState->shaftCullStrategy);
            *candidatesList = arr;
        } else {
            java::ArrayList<Geometry*> *arr = new java::ArrayList<Geometry*>();
            shaft.doCulling(*candidatesList, arr, galerkinState->shaftCullStrategy);
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
    const GalerkinState *galerkinState)
{
    if ( galerkinState->shaftCullMode == GalerkinShaftCullMode::DO_SHAFT_CULLING_FOR_REFINEMENT ||
         galerkinState->shaftCullMode == GalerkinShaftCullMode::ALWAYS_DO_SHAFT_CULLING ) {
        Shaft::freeCandidateList(*candidatesList);
    }
}

/**
Link error estimation
*/

double
HierarchicalRefinementStrategy::hierarchicRefinementColorToError(ColorRgb radiance) {
    return radiance.maximumComponent();
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
    const Interaction *interaction,
    const double receiverArea,
    const GalerkinState *galerkinState) {
    double threshold;

    switch ( galerkinState->errorNorm ) {
        case GalerkinErrorNorm::RADIANCE_ERROR:
            threshold = hierarchicRefinementColorToError(
                GLOBAL_statistics.maxSelfEmittedRadiance) * galerkinState->relLinkErrorThreshold;
            break;
        case GalerkinErrorNorm::POWER_ERROR:
            threshold = hierarchicRefinementColorToError(
                GLOBAL_statistics.maxSelfEmittedPower) * galerkinState->relLinkErrorThreshold / (M_PI * receiverArea);
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
    if ( galerkinState->importanceDriven &&
         (galerkinState->galerkinIterationMethod == GalerkinIterationMethod::JACOBI ||
          galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL) ) {
        threshold /= 2.0 * interaction->receiverElement->potential / GLOBAL_statistics.maxDirectPotential;
    }

    return threshold;
}

/**
Compute an estimate for the approximation error that would be made if the
candidate interaction were used for light transport. Use the sources un-shot
radiance when doing shooting and the total radiance when gathering
*/
double
HierarchicalRefinementStrategy::hierarchicRefinementApproximationError(
    Interaction *interaction,
    ColorRgb srcRho,
    ColorRgb rcvRho,
    GalerkinState *galerkinState)
{
    ColorRgb error;
    ColorRgb srcRad;
    double approxError;
    double approxError2;

    switch ( galerkinState->galerkinIterationMethod ) {
        case GalerkinIterationMethod::GAUSS_SEIDEL:
        case GalerkinIterationMethod::JACOBI:
            if ( interaction->sourceElement->isCluster()
              && interaction->sourceElement != interaction->receiverElement ) {
                srcRad = ClusterTraversalStrategy::maxRadiance(interaction->sourceElement, galerkinState);
            } else {
                srcRad = interaction->sourceElement->radiance[0];
            }

            error.scalarProductScaled(rcvRho, interaction->deltaK[0], srcRad);
            error.abs();
            approxError = hierarchicRefinementColorToError(error);
            break;

        case GalerkinIterationMethod::SOUTH_WELL:
            if ( interaction->sourceElement->isCluster()
              && interaction->sourceElement != interaction->receiverElement ) {
                // Returns un-shot radiance for shooting
                srcRad = ClusterTraversalStrategy::sourceClusterRadiance(interaction, galerkinState);
            } else {
                srcRad = interaction->sourceElement->unShotRadiance[0];
            }

            error.scalarProductScaled(rcvRho, interaction->deltaK[0], srcRad);
            error.abs();
            approxError = hierarchicRefinementColorToError(error);

            if ( galerkinState->importanceDriven && interaction->receiverElement->isCluster() ) {
                // Make sure the interaction is also suited for transport of un-shot potential
                // from source to receiver. Note that it makes no sense to
                // subdivide receiver patches (potential is only used to help
                // choosing a radiance shooting patch
                approxError2 = (hierarchicRefinementColorToError(srcRho)
                                * interaction->deltaK[0] * interaction->sourceElement->unShotPotential);

                // Compare potential error w.r.t. maximum direct potential or importance
                // instead of self-emitted radiance or power
                switch ( galerkinState->errorNorm ) {
                    case GalerkinErrorNorm::RADIANCE_ERROR:
                        approxError2 *= hierarchicRefinementColorToError(
                            GLOBAL_statistics.maxSelfEmittedRadiance) / GLOBAL_statistics.maxDirectPotential;
                        break;
                    case GalerkinErrorNorm::POWER_ERROR:
                        approxError2 *= hierarchicRefinementColorToError(
                            GLOBAL_statistics.maxSelfEmittedPower) / M_PI / GLOBAL_statistics.maxDirectImportance;
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
HierarchicalRefinementStrategy::sourceClusterRadianceVariationError(
    Interaction *interaction,
    ColorRgb rcvRho,
    double receiverArea,
    GalerkinState *galerkinState)
{
    double K = interaction->K[0];
    if ( K == 0.0 || rcvRho.isBlack() || interaction->sourceElement->radiance[0].isBlack() ) {
        // Receiver reflectivity or coupling coefficient or source radiance is zero
        return 0.0;
    }

    Vector3D rcVertices[8];
    int numberOfRcVertices = interaction->receiverElement->vertices(rcVertices, 8);

    ColorRgb minimumSrcRad;
    ColorRgb maximumSrcRad;
    ColorRgb error;

    minimumSrcRad.setMonochrome(Numeric::HUGE_FLOAT_VALUE);
    maximumSrcRad.setMonochrome(-Numeric::HUGE_FLOAT_VALUE);
    for ( int i = 0; i < numberOfRcVertices; i++ ) {
        ColorRgb rad;
        rad = ClusterTraversalStrategy::clusterRadianceToSamplePoint(
                interaction->sourceElement, rcVertices[i], galerkinState);
        minimumSrcRad.minimum(minimumSrcRad, rad);
        maximumSrcRad.maximum(maximumSrcRad, rad);
    }
    error.subtract(maximumSrcRad, minimumSrcRad);

    error.scalarProductScaled(rcvRho, (float)(K / receiverArea), error);
    error.abs();
    return hierarchicRefinementColorToError(error);
}

InteractionEvaluationCode
HierarchicalRefinementStrategy::hierarchicRefinementEvaluateInteraction(
    Interaction *interaction,
    GalerkinState *galerkinState)
{
    ColorRgb srcRho;
    ColorRgb rcvRho;
    double error;
    double threshold;
    double receiveArea;
    double minimumArea;
    InteractionEvaluationCode code;

    if ( !galerkinState->hierarchical ) {
        // Simply don't refine
        return InteractionEvaluationCode::ACCURATE_ENOUGH;
    }

    // Determine receiver area (projected visible area for a receiver cluster)
    // and reflectivity
    if ( interaction->receiverElement->isCluster() ) {
        rcvRho.setMonochrome(1.0);
        receiveArea = ClusterTraversalStrategy::receiverArea(interaction, galerkinState);
    } else {
        rcvRho = interaction->receiverElement->patch->radianceData->Rd;
        receiveArea = interaction->receiverElement->area;
    }

    // Determine source reflectivity
    if ( interaction->sourceElement->isCluster() ) {
        srcRho.setMonochrome(1.0f);
    } else {
        srcRho = interaction->sourceElement->patch->radianceData->Rd;
    }

    // Determine error estimate and error threshold
    threshold = hierarchicRefinementLinkErrorThreshold(interaction, receiveArea, galerkinState);
    error = hierarchicRefinementApproximationError(interaction, srcRho, rcvRho, galerkinState);

    if ( interaction->sourceElement->isCluster()
         && error < threshold
         && galerkinState->clusteringStrategy != GalerkinClusteringStrategy::ISOTROPIC ) {
        error += sourceClusterRadianceVariationError(interaction, rcvRho, receiveArea, galerkinState);
    }

    // Minimal element area for which subdivision is allowed
    minimumArea = GLOBAL_statistics.totalArea * galerkinState->relMinElemArea;

    code = InteractionEvaluationCode::ACCURATE_ENOUGH;
    if ( error > threshold ) {
        // A very simple but robust subdivision strategy: subdivide the
        // largest of the two elements in order to reduce the error
        if ( (!(interaction->sourceElement->isCluster()
            && (interaction->sourceElement->flags & ElementFlags::IS_LIGHT_SOURCE_MASK)) )
            && (receiveArea > interaction->sourceElement->area) ) {
            if ( receiveArea > minimumArea ) {
                if ( interaction->receiverElement->isCluster() ) {
                    code = InteractionEvaluationCode::SUBDIVIDE_RECEIVER_CLUSTER;
                } else {
                    code = InteractionEvaluationCode::REGULAR_SUBDIVIDE_RECEIVER;
                }
            }
        } else {
            if ( interaction->sourceElement->isCluster() ) {
                code = InteractionEvaluationCode::SUBDIVIDE_SOURCE_CLUSTER;
            } else if ( interaction->sourceElement->area > minimumArea ) {
                code = InteractionEvaluationCode::REGULAR_SUBDIVIDE_SOURCE;
            }
        }
    }

    return code;
}

/**
Light transport computation
Computes light transport over the given interaction, which is supposed to be
accurate enough for doing so. Renormalisation and reflection and such is done
once for all accumulated received radiance during push-pull
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementComputeLightTransport(
    Interaction *interaction,
    GalerkinState *galerkinState)
{
    // Update the number of effectively used radiance coefficients on the
    // receiver element
    int a = java::Math::min(interaction->numberOfBasisFunctionsOnReceiver, interaction->receiverElement->basisSize);
    int b = java::Math::min(interaction->numberOfBasisFunctionsOnSource, interaction->sourceElement->basisSize);
    if ( a > interaction->receiverElement->basisUsed ) {
        interaction->receiverElement->basisUsed = (char)a;
    }
    if ( b > interaction->sourceElement->basisUsed ) {
        interaction->sourceElement->basisUsed = (char)b;
    }

    ColorRgb *srcRad;
    ColorRgb *rcvRad;
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        srcRad = interaction->sourceElement->unShotRadiance;
    } else {
        srcRad = interaction->sourceElement->radiance;
    }

    ColorRgb linkClusterRad;
    if ( interaction->sourceElement->isCluster() && interaction->sourceElement != interaction->receiverElement ) {
        linkClusterRad = ClusterTraversalStrategy::sourceClusterRadiance(interaction, galerkinState);
        srcRad = &linkClusterRad;
    }

    if ( interaction->receiverElement->isCluster() && interaction->sourceElement != interaction->receiverElement ) {
        ClusterTraversalStrategy::gatherRadiance(interaction, srcRad, galerkinState);
    } else {
        rcvRad = interaction->receiverElement->receivedRadiance;
        if ( interaction->numberOfBasisFunctionsOnReceiver == 1 && interaction->numberOfBasisFunctionsOnSource == 1 ) {
            rcvRad[0].addScaled(rcvRad[0], interaction->K[0], srcRad[0]);
        } else {
            for ( int alpha = 0; alpha < a; alpha++ ) {
                for ( int beta = 0; beta < b; beta++ ) {
                    rcvRad[alpha].addScaled(
                            rcvRad[alpha],
                            interaction->K[alpha * interaction->numberOfBasisFunctionsOnSource + beta],
                            srcRad[beta]);
                }
            }
        }
    }

    if ( galerkinState->importanceDriven ) {
        float K = interaction->K[0];
        ColorRgb rcvRho;
        ColorRgb srcRho;

        if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL ||
             galerkinState->galerkinIterationMethod == GalerkinIterationMethod::JACOBI ) {
            if ( interaction->receiverElement->isCluster() ) {
                rcvRho.setMonochrome(1.0f);
            } else {
                rcvRho = interaction->receiverElement->patch->radianceData->Rd;
            }
            interaction->sourceElement->receivedPotential +=
                (float)(K * hierarchicRefinementColorToError(rcvRho) * interaction->receiverElement->potential);
        } else if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
            if ( interaction->sourceElement->isCluster() ) {
                srcRho.setMonochrome(1.0f);
            } else {
                srcRho = interaction->sourceElement->patch->radianceData->Rd;
            }
            interaction->receiverElement->receivedPotential +=
                (float)(K * hierarchicRefinementColorToError(srcRho) * interaction->sourceElement->unShotPotential);
        } else {
            logFatal(
                -1, "hierarchicRefinementComputeLightTransport", "Did you introduce a new iteration method or so??");
        }
    }
}

/**
Refinement procedures
*/

/**
Computes the form factor and error estimation coefficients. If the form factor
is not zero, the data is filled in the INTERACTION pointed to by 'interaction'
and true is returned. If the elements don't interact, false is returned
*/
int
HierarchicalRefinementStrategy::hierarchicRefinementCreateSubdivisionLink(
    const Scene *scene,
    const java::ArrayList<Geometry *> *candidatesList,
    GalerkinElement *rcv,
    GalerkinElement *src,
    Interaction *interaction,
    const GalerkinState *galerkinState)
{
    interaction->receiverElement = rcv;
    interaction->sourceElement = src;

    // Always a constant approximation on cluster elements
    if ( interaction->receiverElement->isCluster() ) {
        interaction->numberOfBasisFunctionsOnReceiver = 1;
    } else {
        interaction->numberOfBasisFunctionsOnReceiver = rcv->basisSize;
    }

    if ( interaction->sourceElement->isCluster() ) {
        interaction->numberOfBasisFunctionsOnSource = 1;
    } else {
        interaction->numberOfBasisFunctionsOnSource = src->basisSize;
    }

    const bool isSceneGeometry = (candidatesList == scene->geometryList);
    const bool isClusteredGeometry = (candidatesList == scene->clusteredGeometryList);
    FormFactorStrategy::computeAreaToAreaFormFactorVisibility(
            (const VoxelGrid *)scene->voxelGrid,
            candidatesList,
            isSceneGeometry,
            isClusteredGeometry,
            interaction,
            galerkinState);

    return interaction->visibility != 0;
}

/**
Duplicates the INTERACTION data and stores it with the receivers interactions
if doing gathering and with the source for shooting
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementStoreInteraction(
    Interaction *interaction,
    const GalerkinState *galerkinState)
{
    Interaction *newInteraction = Interaction::interactionDuplicate(interaction);

    if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
        interaction->sourceElement->interactions->add(newInteraction);
    } else {
        interaction->receiverElement->interactions->add(newInteraction);
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
    const Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *interaction,
    bool isClusteredGeometry,
    GalerkinState *galerkinState)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = interaction->sourceElement;
    GalerkinElement *receiverElement = interaction->receiverElement;

    sourceElement->regularSubDivide();
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
                galerkinState) &&
            ( !refineRecursive(
                    scene,
                    candidatesList,
                    &subInteraction,
                    galerkinState) ) ) {
            hierarchicRefinementStoreInteraction(&subInteraction, galerkinState);
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
    const Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *link,
    bool isClusteredGeometry,
    GalerkinState *galerkinState)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, link, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = link->sourceElement;
    GalerkinElement *receiverElement = link->receiverElement;

    receiverElement->regularSubDivide();
    for ( int i = 0; i < 4; i++ ) {
        Interaction subInteraction{};
        GalerkinElement *child = (GalerkinElement *)receiverElement->regularSubElements[i];
        subInteraction.K = new float[MAX_BASIS_SIZE * MAX_BASIS_SIZE];

        if ( hierarchicRefinementCreateSubdivisionLink(
                scene,
                *candidatesList,
                child, sourceElement,
                &subInteraction,
                galerkinState) &&
               !refineRecursive(
                    scene,
                    candidatesList,
                    &subInteraction,
                    galerkinState) ) {
            hierarchicRefinementStoreInteraction(&subInteraction, galerkinState);
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
    const Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *interaction,
    bool isClusteredGeometry,
    GalerkinState *galerkinState)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    const GalerkinElement *sourceElement = interaction->sourceElement;
    GalerkinElement *receiverElement = interaction->receiverElement;

    for ( int i = 0;
          sourceElement->irregularSubElements != nullptr && i < sourceElement->irregularSubElements->size();
          i++ ) {
        GalerkinElement *childElement = (GalerkinElement *)sourceElement->irregularSubElements->get(i);
        Interaction subInteraction{};
        subInteraction.K = new float[MAX_BASIS_SIZE * MAX_BASIS_SIZE];

        if ( !childElement->isCluster() ) {
            const Patch *thePatch = childElement->patch;
            if ( (receiverElement->isCluster()
              && receiverElement->geometry->getBoundingBox().behindPlane(&thePatch->normal, thePatch->planeConstant)) ||
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
                galerkinState) &&
             !refineRecursive(
                scene,
                candidatesList,
                &subInteraction,
                galerkinState) ) {
            hierarchicRefinementStoreInteraction(&subInteraction, galerkinState);
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
    const Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *interaction,
    bool isClusteredGeometry,
    GalerkinState *galerkinState)
{
    java::ArrayList<Geometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = interaction->sourceElement;
    const GalerkinElement *receiverElement = interaction->receiverElement;

    for ( int i = 0;
          receiverElement->irregularSubElements != nullptr && i < receiverElement->irregularSubElements->size();
          i++ ) {
        GalerkinElement *child = (GalerkinElement *)receiverElement->irregularSubElements->get(i);
        Interaction subInteraction{};
        subInteraction.K = new float [MAX_BASIS_SIZE * MAX_BASIS_SIZE];

        if ( !child->isCluster() ) {
            const Patch *thePatch = child->patch;
            if ( (sourceElement->isCluster()
               && sourceElement->geometry->getBoundingBox().behindPlane(&thePatch->normal, thePatch->planeConstant)) ||
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
                galerkinState) &&
              !refineRecursive(
                    scene,
                    candidatesList,
                    &subInteraction,
                    galerkinState) ) {
            hierarchicRefinementStoreInteraction(&subInteraction, galerkinState);
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
    const Scene *scene,
    java::ArrayList<Geometry *> **candidatesList,
    Interaction *interaction,
    GalerkinState *galerkinState)
{
    bool refined;

    bool isClusteredGeometry = (*candidatesList == scene->clusteredGeometryList);
    switch ( hierarchicRefinementEvaluateInteraction(interaction, galerkinState) ) {
        case InteractionEvaluationCode::ACCURATE_ENOUGH:
            hierarchicRefinementComputeLightTransport(interaction, galerkinState);
            refined = false;
            break;
        case InteractionEvaluationCode::REGULAR_SUBDIVIDE_SOURCE:
            hierarchicRefinementRegularSubdivideSource(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
            refined = true;
            break;
        case InteractionEvaluationCode::REGULAR_SUBDIVIDE_RECEIVER:
            hierarchicRefinementRegularSubdivideReceiver(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
            refined = true;
            break;
        case InteractionEvaluationCode::SUBDIVIDE_SOURCE_CLUSTER:
            hierarchicRefinementSubdivideSourceCluster(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
            refined = true;
            break;
        case InteractionEvaluationCode::SUBDIVIDE_RECEIVER_CLUSTER:
            hierarchicRefinementSubdivideReceiverCluster(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
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
HierarchicalRefinementStrategy::HierarchicalRefinementStrategy::refineInteraction(
    const Scene *scene,
    Interaction *interaction,
    GalerkinState *galerkinState)
{
    java::ArrayList<Geometry *> *candidateOccluderList = scene->clusteredGeometryList;

    if ( galerkinState->exactVisibility && interaction->visibility == 255 ) {
        candidateOccluderList = nullptr;
    }

    // We know for sure that there is full visibility
    return refineRecursive(scene, &candidateOccluderList, interaction, galerkinState);
}

void
HierarchicalRefinementStrategy::removeRefinedInteractions(
    const GalerkinState *galerkinState,
    const java::ArrayList<Interaction *> *interactionsToRemove)
{
    for ( int i = 0; i < interactionsToRemove->size(); i++ ) {
        Interaction *interaction = interactionsToRemove->get(i);
        if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
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
    const Scene *scene,
    const GalerkinElement *parentElement,
    GalerkinState *galerkinState)
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
            galerkinState);
    }

    if ( parentElement->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            refineInteractions(
                scene,
                (GalerkinElement *)parentElement->regularSubElements[i],
                galerkinState);
        }
    }

    // Iterate over the interactions. Interactions that are refined are removed from the list
    java::ArrayList<Interaction *> *interactionsToRemove = new java::ArrayList<Interaction *>();

    for ( int i = 0; parentElement->interactions != nullptr && i < parentElement->interactions->size(); i++ ) {
        Interaction *interaction = parentElement->interactions->get(i);
        if ( refineInteraction(scene, interaction, galerkinState) ) {
            interactionsToRemove->add(interaction);
        }
    }
    removeRefinedInteractions(galerkinState, interactionsToRemove);
    delete interactionsToRemove;
}
