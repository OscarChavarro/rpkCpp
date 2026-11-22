/**
Hierarchical refinement
*/

#include <cstdint>
#include <unistd.h>

#include "java/lang/Float.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/lang/Runnable.h"
#include "java/lang/Thread.h"
#include "java/util/ArrayList.txx"
#include "java/util/concurrent/atomic/AtomicLong.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/galerkin/processing/ClusterTraversalStrategy.h"
#include "vsdk/toolkit/galerkin/processing/FormFactorStrategy.h"
#include "vsdk/toolkit/galerkin/processing/HierarchicalRefinementStrategy.h"
#include "vsdk/toolkit/galerkin/Shaft.h"

MemoryPool<float> HierarchicalRefinementStrategy::hierarchicalCoefficientsPool;
bool HierarchicalRefinementStrategy::hierarchicalCoefficientsPoolInitialized = false;

void
HierarchicalRefinementStrategy::ensureHierarchicalCoefficientsPool() {
    if ( !hierarchicalCoefficientsPoolInitialized ) {
        hierarchicalCoefficientsPool.init(16 * 1024 * 1024);
        hierarchicalCoefficientsPoolInitialized = true;
    }
}

void
HierarchicalRefinementStrategy::prepareRefinementIterationState(GalerkinState *galerkinState) {
    const Statistics &statistics = Statistics::instance();
    galerkinState->refinementMinimumArea =
        statistics.radiance.totalArea * galerkinState->relMinElemArea;
    galerkinState->refinementRadianceErrorThreshold =
        hierarchicRefinementColorToError(statistics.radiance.maxSelfEmittedRadiance)
        * galerkinState->relLinkErrorThreshold;
    galerkinState->refinementPowerErrorThresholdNumerator =
        hierarchicRefinementColorToError(statistics.radiance.maxSelfEmittedPower)
        * galerkinState->relLinkErrorThreshold;
    galerkinState->refinementMaxDirectPotential =
        statistics.potential.maxDirectPotential;
}

/**
Does shaft-culling between elements in a interaction (if the user asked for it).
Updates the *candidatesList. Returns the old candidate list, so it can be restored
later (using hierarchicRefinementUnCull())
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementCull(
    const Scene *scene,
    java::ArrayList<RpkGeometry *> **candidatesList,
    Interaction *interaction,
    bool isClusteredGeometry,
    const GalerkinState *galerkinState)
{
    if ( *candidatesList == nullptr ) {
        return;
    }

    if ( galerkinState->shaftCullMode == GalerkinShaftCullMode::DO_SHAFT_CULLING_FOR_REFINEMENT ||
         galerkinState->shaftCullMode == GalerkinShaftCullMode::ALWAYS_DO_SHAFT_CULLING ) {
        Shaft shaft(scene->getFaceBounds());
        // Keep these temporaries alive during the full culling pass because Shaft
        // stores references used later in cullGeometry/doCulling.
        Polygon rcvPolygon;
        Polygon srcPolygon;
        RpkAxisAlignedBoundingBox srcBounds;
        RpkAxisAlignedBoundingBox rcvBounds;

        if ( galerkinState->exactVisibility
          && !interaction->receiverElement->isCluster()
          && !interaction->sourceElement->isCluster() ) {
            interaction->receiverElement->initPolygon(&rcvPolygon);
            interaction->sourceElement->initPolygon(&srcPolygon);
            shaft.constructFromPolygonToPolygon(&rcvPolygon, &srcPolygon);
        } else {
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
            java::ArrayList<RpkGeometry*> *arr = new java::ArrayList<RpkGeometry*>();
            shaft.cullGeometry(scene->clusteredRootGeometry, arr, galerkinState->shaftCullStrategy);
            *candidatesList = arr;
        } else {
            java::ArrayList<RpkGeometry*> *arr = new java::ArrayList<RpkGeometry*>();
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
    java::ArrayList<RpkGeometry *> **candidatesList,
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
HierarchicalRefinementStrategy::hierarchicRefinementColorToError(ColorRgbMutable radiance) {
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
    double threshold = 0.0;

    switch ( galerkinState->errorNorm ) {
        case GalerkinErrorNorm::RADIANCE_ERROR:
            threshold = galerkinState->refinementRadianceErrorThreshold;
            break;
        case GalerkinErrorNorm::POWER_ERROR:
            threshold = galerkinState->refinementPowerErrorThresholdNumerator / (M_PI * receiverArea);
            break;
        default:
            Logger::reportMessage("HierarchicalRefinementStrategy", Logger::FATAL_ERROR, "hierarchicRefinementEvaluateInteraction", "Invalid error norm");
    }

    // Weight the error with the potential of the receiver in case of view-potential
    // driven gathering (potential is used in a totally different way in shooting
    // algorithms). It is assumed that the average direct potential is about half
    // of the maximum direct potential. This way, about an equal precision is
    // obtained in the visible parts of the scene if importance is used
    // instead of weighting the error, we weight the threshold with the inverse
    if ( galerkinState->importanceDriven &&
         (galerkinState->isJacobi() ||
          galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL) ) {
        threshold /= 2.0 * interaction->receiverElement->potential / galerkinState->refinementMaxDirectPotential;
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
    ColorRgbMutable srcRho,
    ColorRgbMutable rcvRho,
    GalerkinState *galerkinState)
{
    ColorRgbMutable error(0.0, 0.0, 0.0);
    ColorRgbMutable srcRad(0.0, 0.0, 0.0);
    double approxError = 0.0;
    double approxError2;

    switch ( galerkinState->galerkinIterationMethod ) {
        case GalerkinIterationMethod::GAUSS_SEIDEL:
        case GalerkinIterationMethod::JACOBI:
        case GalerkinIterationMethod::PARALLEL_JACOBI:
            if ( interaction->sourceElement->isCluster()
              && interaction->sourceElement != interaction->receiverElement ) {
                srcRad = ClusterTraversalStrategy::maxRadiance(interaction->sourceElement, galerkinState);
            } else {
                srcRad = interaction->sourceElement->radiance[0];
            }

            error.scalarProductScaled(rcvRho, interaction->deltaK, srcRad);
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

            error.scalarProductScaled(rcvRho, interaction->deltaK, srcRad);
            error.abs();
            approxError = hierarchicRefinementColorToError(error);

            if ( galerkinState->importanceDriven && interaction->receiverElement->isCluster() ) {
                // Make sure the interaction is also suited for transport of un-shot potential
                // from source to receiver. Note that it makes no sense to
                // subdivide receiver patches (potential is only used to help
                // choosing a radiance shooting patch
                approxError2 = (hierarchicRefinementColorToError(srcRho)
                                * interaction->deltaK * interaction->sourceElement->unShotPotential);

                // Compare potential error w.r.t. maximum direct potential or importance
                // instead of self-emitted radiance or power
                switch ( galerkinState->errorNorm ) {
                    case GalerkinErrorNorm::RADIANCE_ERROR:
                        approxError2 *= hierarchicRefinementColorToError(
                            Statistics::instance().radiance.maxSelfEmittedRadiance) / Statistics::instance().potential.maxDirectPotential;
                        break;
                    case GalerkinErrorNorm::POWER_ERROR:
                        approxError2 *= hierarchicRefinementColorToError(
                            Statistics::instance().radiance.maxSelfEmittedPower) / M_PI / Statistics::instance().potential.maxDirectImportance;
                        break;
                }
                if ( approxError2 > approxError ) {
                    approxError = approxError2;
                }
            }
            break;

        default:
            Logger::reportMessage("HierarchicalRefinementStrategy", Logger::FATAL_ERROR, "hierarchicRefinementApproximationError", "Invalid iteration method");
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
    ColorRgbMutable rcvRho,
    double receiverArea,
    GalerkinState *galerkinState)
{
    double K = interaction->K[0];
    if ( K == 0.0 || rcvRho.isBlack() || interaction->sourceElement->radiance[0].isBlack() ) {
        // Receiver reflectivity or coupling coefficient or source radiance is zero
        return 0.0;
    }

    Vector3Df rcVertices[8];
    int numberOfRcVertices = interaction->receiverElement->vertices(rcVertices);

    ColorRgbMutable minimumSrcRad(0.0, 0.0, 0.0);
    ColorRgbMutable maximumSrcRad(0.0, 0.0, 0.0);
    ColorRgbMutable error(0.0, 0.0, 0.0);

    minimumSrcRad = ColorRgbMutable(java::Float::MAX_VALUE, java::Float::MAX_VALUE, java::Float::MAX_VALUE);
    maximumSrcRad = ColorRgbMutable(-java::Float::MAX_VALUE, -java::Float::MAX_VALUE, -java::Float::MAX_VALUE);
    for ( int i = 0; i < numberOfRcVertices; i++ ) {
        ColorRgbMutable rad(0.0, 0.0, 0.0);
        rad = ClusterTraversalStrategy::clusterRadianceToSamplePoint(
                interaction->sourceElement, rcVertices[i], galerkinState);
        minimumSrcRad.minimum(minimumSrcRad, rad);
        maximumSrcRad.maximum(maximumSrcRad, rad);
    }
    error.subtract(maximumSrcRad, minimumSrcRad);

    error.scalarProductScaled(rcvRho, static_cast<float>(K / receiverArea), error);
    error.abs();
    return hierarchicRefinementColorToError(error);
}

InteractionEvaluationCode
HierarchicalRefinementStrategy::hierarchicRefinementEvaluateInteraction(
    Interaction *interaction,
    GalerkinState *galerkinState)
{
    ColorRgbMutable srcRho(0.0, 0.0, 0.0);
    ColorRgbMutable rcvRho(0.0, 0.0, 0.0);
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
        rcvRho = ColorRgbMutable(1.0, 1.0, 1.0);
        receiveArea = ClusterTraversalStrategy::receiverArea(interaction, galerkinState);
    } else {
        rcvRho = interaction->receiverElement->galerkinState->topLevelElement(interaction->receiverElement->patch)->Rd;
        receiveArea = interaction->receiverElement->area;
    }

    // Determine source reflectivity
    if ( interaction->sourceElement->isCluster() ) {
        srcRho = ColorRgbMutable(1.0F, 1.0F, 1.0F);
    } else {
        srcRho = interaction->sourceElement->galerkinState->topLevelElement(interaction->sourceElement->patch)->Rd;
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
    minimumArea = galerkinState->refinementMinimumArea;

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
    receiverLightTransport(interaction, galerkinState);
    sourceLightTransport(interaction, galerkinState);
}

/**
The part of light transport that writes the receiver: the number of
effectively used radiance coefficients and the received radiance. It reads
only the source radiance (frozen during gathering)
*/
void
HierarchicalRefinementStrategy::receiverLightTransport(
    const Interaction *interaction,
    GalerkinState *galerkinState)
{
    // Update the number of effectively used radiance coefficients on the
    // receiver element
    int a = java::Math::min(interaction->numberOfBasisFunctionsOnReceiver, interaction->receiverElement->basisSize);
    int b = java::Math::min(interaction->numberOfBasisFunctionsOnSource, interaction->sourceElement->basisSize);
    if ( a > interaction->receiverElement->basisUsed ) {
        interaction->receiverElement->basisUsed = static_cast<char>(a);
    }

    ColorRgbMutable *srcRad;
    ColorRgbMutable *rcvRad;
    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        srcRad = interaction->sourceElement->unShotRadiance;
    } else {
        srcRad = interaction->sourceElement->radiance;
    }

    ColorRgbMutable linkClusterRad(0.0, 0.0, 0.0);
    if ( interaction->sourceElement->isCluster() && interaction->sourceElement != interaction->receiverElement ) {
        linkClusterRad = ClusterTraversalStrategy::sourceClusterRadiance(const_cast<Interaction *>(interaction), galerkinState);
        srcRad = &linkClusterRad;
    }

    if ( interaction->receiverElement->isCluster() && interaction->sourceElement != interaction->receiverElement ) {
        ClusterTraversalStrategy::gatherRadiance(const_cast<Interaction *>(interaction), srcRad, galerkinState);
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

    if ( galerkinState->importanceDriven && galerkinState->galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
        float K = interaction->K[0];
        ColorRgbMutable srcRho(0.0, 0.0, 0.0);
        if ( interaction->sourceElement->isCluster() ) {
            srcRho = ColorRgbMutable(1.0F, 1.0F, 1.0F);
        } else {
            srcRho = interaction->sourceElement->galerkinState->topLevelElement(interaction->sourceElement->patch)->Rd;
        }
        interaction->receiverElement->receivedPotential +=
            static_cast<float>(K * hierarchicRefinementColorToError(srcRho) * interaction->sourceElement->unShotPotential);
    }
}

/**
The part of light transport that writes the source: the number of effectively
used radiance coefficients and, for importance-driven gathering, the received
potential. It reads only the receiver potential (frozen during gathering)
*/
void
HierarchicalRefinementStrategy::sourceLightTransport(
    const Interaction *interaction,
    const GalerkinState *galerkinState)
{
    int b = java::Math::min(interaction->numberOfBasisFunctionsOnSource, interaction->sourceElement->basisSize);
    if ( b > interaction->sourceElement->basisUsed ) {
        interaction->sourceElement->basisUsed = static_cast<char>(b);
    }

    if ( galerkinState->importanceDriven ) {
        if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::GAUSS_SEIDEL ||
             galerkinState->isJacobi() ) {
            float K = interaction->K[0];
            ColorRgbMutable rcvRho(0.0, 0.0, 0.0);
            if ( interaction->receiverElement->isCluster() ) {
                rcvRho = ColorRgbMutable(1.0F, 1.0F, 1.0F);
            } else {
                rcvRho = interaction->receiverElement->galerkinState->topLevelElement(interaction->receiverElement->patch)->Rd;
            }
            interaction->sourceElement->receivedPotential +=
                static_cast<float>(K * hierarchicRefinementColorToError(rcvRho) * interaction->receiverElement->potential);
        } else if ( galerkinState->galerkinIterationMethod != GalerkinIterationMethod::SOUTH_WELL ) {
            Logger::reportMessage("HierarchicalRefinementStrategy", Logger::FATAL_ERROR, "hierarchicRefinementComputeLightTransport", "Did you introduce a new iteration method or so??");
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
    const java::ArrayList<RpkGeometry *> *candidatesList,
    GalerkinElement *receiverElement,
    GalerkinElement *sourceElement,
    Interaction *interaction,
    const GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    interaction->receiverElement = receiverElement;
    interaction->sourceElement = sourceElement;

    // Always a constant approximation on cluster elements
    if ( interaction->receiverElement->isCluster() ) {
        interaction->numberOfBasisFunctionsOnReceiver = 1;
    } else {
        interaction->numberOfBasisFunctionsOnReceiver = receiverElement->basisSize;
    }

    if ( interaction->sourceElement->isCluster() ) {
        interaction->numberOfBasisFunctionsOnSource = 1;
    } else {
        interaction->numberOfBasisFunctionsOnSource = sourceElement->basisSize;
    }

    const bool isSceneGeometry = (candidatesList == scene->geometryList);
    const bool isClusteredGeometry = (candidatesList == scene->clusteredGeometryList);
    FormFactorStrategy::computeAreaToAreaFormFactorVisibility(
        scene->getSceneQuery(),
        workspace->getSceneQueryWorkspace(),
        workspace->getFormFactorWorkspace(),
        candidatesList,
        isSceneGeometry,
        isClusteredGeometry,
        interaction,
        galerkinState);

    // Warnings kept by a parallel worker become events, in their order
    RefinementRecord *record = workspace->getRecord();
    if ( record != nullptr ) {
        java::ArrayList<const GalerkinElement *> *pairs = record->getNodesTooClose();
        for ( long int i = 0; i + 1 < pairs->size(); i += 2 ) {
            record->getEvents()->add(RefinementEvent::nodesTooClose(pairs->get(i), pairs->get(i + 1)));
        }
        pairs->clear();
    }

    return interaction->visibility != 0;
}

/**
Duplicates the INTERACTION data and stores it with the receivers interactions
if doing gathering and with the source for shooting
*/
void
HierarchicalRefinementStrategy::hierarchicRefinementStoreInteraction(
    Interaction *interaction,
    const GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    RefinementRecord *record = workspace->getRecord();
    if ( record != nullptr ) {
        // Parallel Jacobi: counted, stored and transported over when committed
        record->getEvents()->add(RefinementEvent::newLink(Interaction::uncountedDuplicate(interaction)));
        return;
    }

    Interaction *newInteraction = Interaction::interactionDuplicate(interaction);

    if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
        interaction->sourceElement->interactions->add(newInteraction);
    } else {
        interaction->receiverElement->interactions->add(newInteraction);
    }
}

/**
Regular subdivision of a surface element. Parallel Jacobi workers take the
shared lock (subdivision is idempotent, and the children only depend on the
frozen state of the parent) and record the event: the ids of new children are
0 until the commit gives them the ids of the serial order
*/
void
HierarchicalRefinementStrategy::regularSubDivide(GalerkinElement *element, RefinementWorkspace *workspace) {
    java::ReentrantLock *lock = workspace->getSubdivisionLock();
    if ( lock == nullptr ) {
        element->regularSubDivide();
        return;
    }

    lock->lock();
    if ( element->regularSubElements == nullptr ) {
        element->regularSubDivide();
        for ( int i = 0; i < 4; i++ ) {
            element->regularSubElements[i]->id = 0;
        }
    }
    lock->unlock();
    workspace->getRecord()->getEvents()->add(RefinementEvent::subdivision(element));
}

/**
Creates the interaction between the receiver and the source (sub-elements of
a refined interaction), refines it and, if it needs no further refinement,
stores it. A parallel Jacobi worker may instead leave it to another worker as
a new task: the FORK event keeps its place in the serial order
*/
void
HierarchicalRefinementStrategy::refineOrForkSubInteraction(
    const Scene *scene,
    java::ArrayList<RpkGeometry *> *candidatesList,
    GalerkinElement *receiverElement,
    GalerkinElement *sourceElement,
    GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    RefinementScheduler *scheduler = workspace->getScheduler();
    if ( scheduler != nullptr && scheduler->wantsMoreTasks() ) {
        const long int task = scheduler->fork(new RefinementTask(receiverElement, sourceElement, candidatesList));
        workspace->getRecord()->getEvents()->add(RefinementEvent::fork(task));
        return;
    }
    refineSubInteraction(scene, candidatesList, receiverElement, sourceElement, galerkinState, workspace);
}

void
HierarchicalRefinementStrategy::refineSubInteraction(
    const Scene *scene,
    java::ArrayList<RpkGeometry *> *candidatesList,
    GalerkinElement *receiverElement,
    GalerkinElement *sourceElement,
    GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    Interaction subInteraction{};
    constexpr int KSize = GalerkinBasis::MAX_BASIS_SIZE * GalerkinBasis::MAX_BASIS_SIZE;
    MemoryPool<float> *coefficientsPool = workspace->getCoefficientsPool();
    bool usingPool = true;
    subInteraction.K = coefficientsPool->allocate(KSize);
    if ( subInteraction.K == nullptr ) {
        if ( coefficientsPool->expand(KSize * 256) ) {
            subInteraction.K = coefficientsPool->allocate(KSize);
        }
        if ( subInteraction.K == nullptr ) {
            usingPool = false;
            subInteraction.K = new float[KSize];
        }
    }
    subInteraction.ownsK = !usingPool;

    java::ArrayList<RpkGeometry *> *candidates = candidatesList;
    if ( hierarchicRefinementCreateSubdivisionLink(
            scene,
            candidates,
            receiverElement,
            sourceElement,
            &subInteraction,
            galerkinState,
            workspace) &&
         !refineRecursive(
            scene,
            &candidates,
            &subInteraction,
            galerkinState,
            workspace) ) {
        hierarchicRefinementStoreInteraction(&subInteraction, galerkinState, workspace);
    }

    if ( usingPool ) {
        subInteraction.K = nullptr;
        coefficientsPool->free(KSize);
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
    java::ArrayList<RpkGeometry *> **candidatesList,
    Interaction *interaction,
    bool isClusteredGeometry,
    GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    java::ArrayList<RpkGeometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = interaction->sourceElement;
    GalerkinElement *receiverElement = interaction->receiverElement;

    regularSubDivide(sourceElement, workspace);
    for ( int i = 0; i < 4; i++ ) {
        GalerkinElement *child = static_cast<GalerkinElement *>(sourceElement->regularSubElements[i]);
        refineOrForkSubInteraction(scene, *candidatesList, receiverElement, child, galerkinState, workspace);
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
    java::ArrayList<RpkGeometry *> **candidatesList,
    Interaction *interaction,
    bool isClusteredGeometry,
    GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    java::ArrayList<RpkGeometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = interaction->sourceElement;
    GalerkinElement *receiverElement = interaction->receiverElement;

    regularSubDivide(receiverElement, workspace);
    for ( int i = 0; i < 4; i++ ) {
        GalerkinElement *child = static_cast<GalerkinElement *>(receiverElement->regularSubElements[i]);
        refineOrForkSubInteraction(scene, *candidatesList, child, sourceElement, galerkinState, workspace);
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
    java::ArrayList<RpkGeometry *> **candidatesList,
    Interaction *interaction,
    bool isClusteredGeometry,
    GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    java::ArrayList<RpkGeometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    const GalerkinElement *sourceElement = interaction->sourceElement;
    GalerkinElement *receiverElement = interaction->receiverElement;

    for ( int i = 0;
          sourceElement->irregularSubElements != nullptr && i < sourceElement->irregularSubElements->size();
          i++ ) {
        GalerkinElement *childElement = static_cast<GalerkinElement *>(sourceElement->irregularSubElements->get(i));
        if ( !childElement->isCluster() ) {
            const Patch *thePatch = childElement->patch;
            if ( (receiverElement->isCluster()
              && receiverElement->geometry->getBoundingBox().behindPlane(thePatch->getNormal(), thePatch->getPlaneConstant())) ||
                (!receiverElement->isCluster() && !receiverElement->patch->facing(thePatch)) ) {
                continue;
            }
        }
        refineOrForkSubInteraction(scene, *candidatesList, receiverElement, childElement, galerkinState, workspace);
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
    java::ArrayList<RpkGeometry *> **candidatesList,
    Interaction *interaction,
    bool isClusteredGeometry,
    GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    java::ArrayList<RpkGeometry *> *backup = *candidatesList;
    hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    GalerkinElement *sourceElement = interaction->sourceElement;
    const GalerkinElement *receiverElement = interaction->receiverElement;

    for ( int i = 0;
          receiverElement->irregularSubElements != nullptr && i < receiverElement->irregularSubElements->size();
          i++ ) {
        GalerkinElement *child = static_cast<GalerkinElement *>(receiverElement->irregularSubElements->get(i));
        if ( !child->isCluster() ) {
            const Patch *thePatch = child->patch;
            if ( (sourceElement->isCluster()
               && sourceElement->geometry->getBoundingBox().behindPlane(thePatch->getNormal(), thePatch->getPlaneConstant())) ||
                (!sourceElement->isCluster() && !sourceElement->patch->facing(thePatch)) ) {
                continue;
            }
        }
        refineOrForkSubInteraction(scene, *candidatesList, child, sourceElement, galerkinState, workspace);
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
    java::ArrayList<RpkGeometry *> **candidatesList,
    Interaction *interaction,
    GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    bool refined = false;

    bool isClusteredGeometry = (*candidatesList == scene->clusteredGeometryList);
    switch ( hierarchicRefinementEvaluateInteraction(interaction, galerkinState) ) {
        case InteractionEvaluationCode::ACCURATE_ENOUGH:
            // A parallel Jacobi worker leaves light transport to the commit
            if ( workspace->getRecord() == nullptr ) {
                hierarchicRefinementComputeLightTransport(interaction, galerkinState);
            }
            refined = false;
            break;
        case InteractionEvaluationCode::REGULAR_SUBDIVIDE_SOURCE:
            hierarchicRefinementRegularSubdivideSource(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace);
            refined = true;
            break;
        case InteractionEvaluationCode::REGULAR_SUBDIVIDE_RECEIVER:
            hierarchicRefinementRegularSubdivideReceiver(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace);
            refined = true;
            break;
        case InteractionEvaluationCode::SUBDIVIDE_SOURCE_CLUSTER:
            hierarchicRefinementSubdivideSourceCluster(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace);
            refined = true;
            break;
        case InteractionEvaluationCode::SUBDIVIDE_RECEIVER_CLUSTER:
            hierarchicRefinementSubdivideReceiverCluster(
                    scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace);
            refined = true;
            break;
        default:
            Logger::reportMessage("HierarchicalRefinementStrategy", Logger::FATAL_ERROR, "refineRecursive", "Invalid result from hierarchicRefinementEvaluateInteraction()");
    }

    return refined;
}

/**
Candidate occluder list for a pair of patches, note it is changed inside the methods!
*/
bool
HierarchicalRefinementStrategy::refineInteraction(
    const Scene *scene,
    Interaction *interaction,
    GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    java::ArrayList<RpkGeometry *> *candidateOccluderList = scene->clusteredGeometryList;

    if ( galerkinState->exactVisibility && interaction->visibility == 255 ) {
        candidateOccluderList = nullptr;
    }

    // We know for sure that there is full visibility
    return refineRecursive(scene, &candidateOccluderList, interaction, galerkinState, workspace);
}

/**
Refines and computes light transport over all interactions of the given
toplevel element
*/
void
HierarchicalRefinementStrategy::refineInteractionsRecursive(
    const Scene *scene,
    const GalerkinElement *parentElement,
    GalerkinState *galerkinState,
    RefinementWorkspace *workspace)
{
    // Interactions will only be replaced by lower level interactions. We try refinement
    // beginning at the lowest levels in the hierarchy and working upwards to
    // prevent already refined interactions from being tested for refinement
    // again
    for ( int i = 0;
          parentElement->irregularSubElements != nullptr && i < parentElement->irregularSubElements->size();
          i++ ) {
        refineInteractionsRecursive(
            scene,
            static_cast<GalerkinElement *>(parentElement->irregularSubElements->get(i)),
            galerkinState,
            workspace);
    }

    if ( parentElement->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            refineInteractionsRecursive(
                scene,
                static_cast<GalerkinElement *>(parentElement->regularSubElements[i]),
                galerkinState,
                workspace);
        }
    }

    // Iterate over the interactions. Refined interactions are removed with
    // order-preserving in-place compaction to avoid one temporary list per element.
    java::ArrayList<Interaction *> *interactions = parentElement->interactions;
    long int writeIndex = 0;
    for ( long int readIndex = 0; interactions != nullptr && readIndex < interactions->size(); readIndex++ ) {
        Interaction *interaction = interactions->get(readIndex);
        if ( refineInteraction(scene, interaction, galerkinState, workspace) ) {
            Interaction::interactionDestroy(interaction);
        } else {
            if ( writeIndex != readIndex ) {
                interactions->set(writeIndex, interaction);
            }
            writeIndex++;
        }
    }
    if ( interactions != nullptr ) {
        while ( interactions->size() > writeIndex ) {
            interactions->remove(interactions->size() - 1);
        }
    }
}

void
HierarchicalRefinementStrategy::refineInteractions(
    const Scene *scene,
    const GalerkinElement *parentElement,
    GalerkinState *galerkinState)
{
    prepareRefinementIterationState(galerkinState);
    ensureHierarchicalCoefficientsPool();
    RefinementWorkspace workspace(
        scene->getSceneQueryWorkspace(),
        galerkinState->formFactorWorkspace,
        &hierarchicalCoefficientsPool,
        nullptr,
        nullptr,
        nullptr);
    refineInteractionsRecursive(scene, parentElement, galerkinState, &workspace);
}

/**
Parallel Jacobi refinement.
With Jacobi iterations, refinement only reads state frozen during the
iteration (radiance, potential, reflectivity, error thresholds), so the
refinement of each interaction is independent of the others, except for what
it changes: received radiance and potential (light transport), interaction
lists (new links), the regular subdivision of elements and their ids, the
shadow ray counters and the warnings. The workers refine and compute form
factors with their own scratch and record those changes; the commit then does
them in the order of the serial refinement (refineInteractionsRecursive), so
that every floating point sum and every list is the serial one.
Interactions are those of the element lists when the iteration starts, in the
serial post-order. The serial refinement of an element also visits the links
that the element's own refinement appends to its list (receiver kept, source
subdivided); the commit visits them serially, as the serial loop does.
*/

bool
HierarchicalRefinementStrategy::parallelRefinementSupported(const Scene *scene, const GalerkinState *galerkinState) {
    return galerkinState->clusteringStrategy == GalerkinClusteringStrategy::ISOTROPIC
        && galerkinState->shaftCullMode == GalerkinShaftCullMode::NEVER_DO_SHAFT_CULLING
        && !galerkinState->multiResolutionVisibility
        && scene->getSceneQueryWorkspace()->getRecorder() == nullptr;
}

int
HierarchicalRefinementStrategy::numberOfRefinementThreads(const GalerkinState *galerkinState) {
    int threads = galerkinState->numberOfThreads;
    if ( threads <= 0 ) {
        threads = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
    }
    return threads < 1 ? 1 : threads;
}

/**
Collects, in the order of refineInteractionsRecursive, the elements having
interactions and their interactions
*/
void
HierarchicalRefinementStrategy::collectRefinementItems(
    const GalerkinElement *parentElement,
    java::ArrayList<RefinementGroup> *groups,
    java::ArrayList<Interaction *> *items)
{
    for ( int i = 0;
          parentElement->irregularSubElements != nullptr && i < parentElement->irregularSubElements->size();
          i++ ) {
        collectRefinementItems(
            static_cast<GalerkinElement *>(parentElement->irregularSubElements->get(i)), groups, items);
    }

    if ( parentElement->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            collectRefinementItems(
                static_cast<GalerkinElement *>(parentElement->regularSubElements[i]), groups, items);
        }
    }

    const java::ArrayList<Interaction *> *interactions = parentElement->interactions;
    if ( interactions != nullptr && interactions->size() > 0 ) {
        groups->add(RefinementGroup(parentElement, items->size(), interactions->size()));
        for ( long int i = 0; i < interactions->size(); i++ ) {
            items->add(interactions->get(i));
        }
    }
}

namespace {

/**
Runs body(w) for w = 0 .. numberOfThreads - 1, each in its own thread, and
waits for all
*/
template<class F>
void
runWorkers(int numberOfThreads, const F &body) {
    java::ArrayList<java::Runnable *> tasks;
    java::ArrayList<java::Thread *> threads;
    for ( int w = 0; w < numberOfThreads; w++ ) {
        tasks.add(new java::Runnable([&body, w]() { body(w); }));
        threads.add(new java::Thread(tasks.get(w)));
    }
    for ( int w = 0; w < numberOfThreads; w++ ) {
        threads.get(w)->start();
    }
    for ( int w = 0; w < numberOfThreads; w++ ) {
        threads.get(w)->join();
        delete threads.get(w);
        delete tasks.get(w);
    }
}

/**
Worker owning the light transport writes of an element
*/
inline int
ownerOf(const GalerkinElement *element, int numberOfThreads) {
    unsigned long long x = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(element)) >> 4;
    x *= 0x9E3779B97F4A7C15ULL;
    return static_cast<int>((x >> 32) % static_cast<unsigned long long>(numberOfThreads));
}

}

/**
Light transport over the interactions, as if done one after the other in
their order. Each element gets its sums in that order: the interactions are
partitioned by the element written (receiver, then in a second pass source),
keeping their order inside each part, and each part goes to one worker
*/
void
HierarchicalRefinementStrategy::applyLightTransports(
    const java::ArrayList<Interaction *> *transports,
    GalerkinState *galerkinState,
    int numberOfThreads)
{
    const long int n = transports->size();
    if ( numberOfThreads <= 1 || n < 4096 ) {
        for ( long int i = 0; i < n; i++ ) {
            hierarchicRefinementComputeLightTransport(transports->get(i), galerkinState);
        }
        return;
    }

    // parts[slice * W + owner]: indices of the slice's interactions, in order
    const int W = numberOfThreads;
    java::ArrayList<java::ArrayList<int> *> receiverParts;
    java::ArrayList<java::ArrayList<int> *> sourceParts;
    for ( long int i = 0; i < static_cast<long int>(W) * W; i++ ) {
        receiverParts.add(new java::ArrayList<int>());
        sourceParts.add(new java::ArrayList<int>());
    }
    const long int sliceSize = (n + W - 1) / W;
    runWorkers(W, [&](int slice) {
        const long int first = slice * sliceSize;
        const long int end = java::Math::min(first + sliceSize, n);
        for ( long int i = first; i < end; i++ ) {
            const Interaction *interaction = transports->get(i);
            receiverParts.get(static_cast<long int>(slice) * W + ownerOf(interaction->receiverElement, W))->add(static_cast<int>(i));
            sourceParts.get(static_cast<long int>(slice) * W + ownerOf(interaction->sourceElement, W))->add(static_cast<int>(i));
        }
    });
    runWorkers(W, [&](int owner) {
        for ( int slice = 0; slice < W; slice++ ) {
            const java::ArrayList<int> *part = receiverParts.get(static_cast<long int>(slice) * W + owner);
            for ( long int k = 0; k < part->size(); k++ ) {
                receiverLightTransport(transports->get(part->get(k)), galerkinState);
            }
        }
    });
    runWorkers(W, [&](int owner) {
        for ( int slice = 0; slice < W; slice++ ) {
            const java::ArrayList<int> *part = sourceParts.get(static_cast<long int>(slice) * W + owner);
            for ( long int k = 0; k < part->size(); k++ ) {
                sourceLightTransport(transports->get(part->get(k)), galerkinState);
            }
        }
    });
    for ( long int i = 0; i < receiverParts.size(); i++ ) {
        delete receiverParts.get(i);
        delete sourceParts.get(i);
    }
}

/**
Does what the refinement task recorded, in its order: ids for new children,
warnings, counting and storing each new link with its receiver, and the events
of the tasks it forked, where it forked them. The interactions to transport
light over (new links, and the interaction itself when it was not refined)
are appended to transports, in the serial order.
@return true when the task was a top-level interaction that was refined, so
that it is to be destroyed
*/
bool
HierarchicalRefinementStrategy::commitRefinementTask(
    const RefinementScheduler *scheduler,
    const RefinementTask *task,
    RefinementRecord *records,
    java::ArrayList<Interaction *> *transports,
    int *nextElementId)
{
    const java::ArrayList<RefinementEvent> *events = records[task->getWorker()].getEvents();

    for ( long int i = task->getFirstEvent(); i < task->getEndEvent(); i++ ) {
        const RefinementEvent event = events->get(i);
        switch ( event.getKind() ) {
            case RefinementEvent::SUBDIVISION: {
                Element **children = event.getReceiver()->regularSubElements;
                for ( int j = 0; j < 4; j++ ) {
                    if ( children[j]->id == 0 ) {
                        children[j]->id = (*nextElementId)++;
                    }
                }
                break;
            }
            case RefinementEvent::NODES_TOO_CLOSE:
                FormFactorStrategy::reportNodesTooClose(event.getReceiver(), event.getSource());
                break;
            case RefinementEvent::LINK: {
                Interaction *link = event.getLink();
                Interaction::countCreated(link);
                transports->add(link);
                link->receiverElement->interactions->add(link);
                break;
            }
            case RefinementEvent::FORK:
                commitRefinementTask(scheduler, scheduler->getForked(event.getTask()), records, transports, nextElementId);
                break;
        }
    }

    if ( task->getInteraction() != nullptr && !task->isRefined() ) {
        transports->add(task->getInteraction());
    }
    return task->isRefined();
}

void
HierarchicalRefinementStrategy::refineInteractionsInParallel(
    const Scene *scene,
    const GalerkinElement *parentElement,
    GalerkinState *galerkinState)
{
    if ( !parallelRefinementSupported(scene, galerkinState) ) {
        if ( galerkinState->iterationNumber <= 1 ) {
            Logger::reportMessage("HierarchicalRefinementStrategy", Logger::WARNING, "refineInteractionsInParallel",
                "Parallel Jacobi needs isotropic clustering, no shaft culling, no multi-resolution visibility "
                "and no ray capture: refining serially");
        }
        refineInteractions(scene, parentElement, galerkinState);
        return;
    }

    prepareRefinementIterationState(galerkinState);

    // The interactions of the element lists are the items, in the serial order
    const int numberOfThreads = numberOfRefinementThreads(galerkinState);
    java::ArrayList<RefinementGroup> groups;
    java::ArrayList<Interaction *> items;
    collectRefinementItems(parentElement, &groups, &items);
    java::ArrayList<RefinementTask *> *itemTasks = new java::ArrayList<RefinementTask *>();
    itemTasks->reserve(items.size());
    for ( long int i = 0; i < items.size(); i++ ) {
        itemTasks->add(new RefinementTask(items.get(i)));
    }
    const long int chunk = java::Math::max(1L, java::Math::min(64L, items.size() / (8L * numberOfThreads)));
    RefinementScheduler scheduler(itemTasks, chunk, 4L * numberOfThreads);

    // Phase 1: workers refine the items, and the sub-interactions subdivisions fork
    const int firstNewElementId = GalerkinElement::getNumberOfElements() + 1;
    java::ReentrantLock subdivisionLock;
    RefinementRecord *records = new RefinementRecord[numberOfThreads];
    ShadowStatistics *shadowStatistics = new ShadowStatistics[numberOfThreads];
    runWorkers(numberOfThreads, [&](int w) {
        SceneQueryWorkspace sceneQueryWorkspace(*scene->getSceneQuery());
        FormFactorWorkspace formFactorWorkspace(&shadowStatistics[w]);
        formFactorWorkspace.setDeferredNodesTooClose(records[w].getNodesTooClose());
        MemoryPool<float> coefficientsPool;
        coefficientsPool.init(16 * 1024 * 1024);
        RefinementWorkspace workspace(
            &sceneQueryWorkspace,
            &formFactorWorkspace,
            &coefficientsPool,
            &records[w],
            &subdivisionLock,
            &scheduler);
        java::ArrayList<RefinementEvent> *events = records[w].getEvents();
        try {
            long int first;
            long int end;
            scheduler.startItems();
            while ( scheduler.takeItems(&first, &end) ) {
                for ( long int i = first; i < end; i++ ) {
                    RefinementTask *task = scheduler.getItem(i);
                    const long int firstEvent = events->size();
                    const bool refined = refineInteraction(scene, task->getInteraction(), galerkinState, &workspace);
                    task->setResult(w, firstEvent, events->size(), refined);
                }
            }
            scheduler.done();
            for ( RefinementTask *task = scheduler.takeForked(); task != nullptr; task = scheduler.takeForked() ) {
                const long int firstEvent = events->size();
                refineSubInteraction(
                    scene,
                    task->getCandidatesList(),
                    task->getReceiver(),
                    task->getSource(),
                    galerkinState,
                    &workspace);
                task->setResult(w, firstEvent, events->size(), false);
                scheduler.done();
            }
        } catch ( ... ) {
            scheduler.fail();
        }
    });
    for ( int w = 0; w < numberOfThreads; w++ ) {
        Statistics::instance().shadow.numberOfShadowRays += shadowStatistics[w].numberOfShadowRays;
        Statistics::instance().shadow.numberOfShadowCacheHits += shadowStatistics[w].numberOfShadowCacheHits;
    }
    delete[] shadowStatistics;
    if ( scheduler.hasFailed() ) {
        delete[] records;
        Logger::reportMessage("HierarchicalRefinementStrategy", Logger::FATAL_ERROR, "refineInteractionsInParallel", "A refinement worker failed");
    }

    // Phase 2: commit in the serial order; light transport is collected, in
    // that order, and done at the end (in parallel, keeping each element's order)
    ensureHierarchicalCoefficientsPool();
    RefinementWorkspace serialWorkspace(
        scene->getSceneQueryWorkspace(),
        galerkinState->formFactorWorkspace,
        &hierarchicalCoefficientsPool,
        nullptr,
        nullptr,
        nullptr);
    java::ArrayList<Interaction *> transports;
    int nextElementId = firstNewElementId;
    for ( long int g = 0; g < groups.size(); g++ ) {
        const RefinementGroup &group = groups[g];
        java::ArrayList<Interaction *> *interactions = group.getElement()->interactions;
        long int writeIndex = 0;
        for ( long int readIndex = 0; readIndex < interactions->size(); readIndex++ ) {
            Interaction *interaction = interactions->get(readIndex);
            bool refined;
            if ( readIndex < group.getNumberOfItems() ) {
                refined = commitRefinementTask(
                    &scheduler, scheduler.getItem(group.getFirstItem() + readIndex), records, &transports, &nextElementId);
            } else if ( hierarchicRefinementEvaluateInteraction(interaction, galerkinState)
                        == InteractionEvaluationCode::ACCURATE_ENOUGH ) {
                // A link appended by this element's refinement, visited as the serial loop does
                transports.add(interaction);
                refined = false;
            } else {
                // Not expected (the link was accurate enough when stored): refine it
                // serially, after the light transport collected so far
                applyLightTransports(&transports, galerkinState, 1);
                transports.clear();
                refined = refineInteraction(scene, interaction, galerkinState, &serialWorkspace);
            }
            if ( refined ) {
                Interaction::interactionDestroy(interaction);
            } else {
                if ( writeIndex != readIndex ) {
                    interactions->set(writeIndex, interaction);
                }
                writeIndex++;
            }
        }
        while ( interactions->size() > writeIndex ) {
            interactions->remove(interactions->size() - 1);
        }
    }
    delete[] records;
    applyLightTransports(&transports, galerkinState, numberOfThreads);

    if ( nextElementId != GalerkinElement::getNumberOfElements() + 1 ) {
        Logger::reportMessage("HierarchicalRefinementStrategy", Logger::FATAL_ERROR, "refineInteractionsInParallel", "Element ids out of the serial order");
    }
}
