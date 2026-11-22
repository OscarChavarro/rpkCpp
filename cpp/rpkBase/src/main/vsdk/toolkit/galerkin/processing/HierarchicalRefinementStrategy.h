#ifndef HIERARCHICAL_REFINE__
#define HIERARCHICAL_REFINE__

#include "vsdk/toolkit/common/memoryManagement/MemoryPool.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"
#include "vsdk/toolkit/galerkin/processing/InteractionEvaluationCode.h"
#include "vsdk/toolkit/galerkin/processing/RefinementWorkspace.h"

/**
Shaft culling stuff for hierarchical refinement
*/
class HierarchicalRefinementStrategy {
  private:
    static MemoryPool<float> hierarchicalCoefficientsPool;
    static bool hierarchicalCoefficientsPoolInitialized;

    static void ensureHierarchicalCoefficientsPool();

    static void prepareRefinementIterationState(GalerkinState *galerkinState);

    static void
    hierarchicRefinementCull(
        const Scene *scene,
        java::ArrayList<RpkGeometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        const GalerkinState *galerkinState);

    static void
    hierarchicRefinementUnCull(
        java::ArrayList<RpkGeometry *> **candidatesList,
        const GalerkinState *galerkinState);

    static double
    hierarchicRefinementColorToError(ColorRgbMutable radiance);

    static double
    hierarchicRefinementLinkErrorThreshold(
        const Interaction *interaction,
        double receiverArea,
        const GalerkinState *galerkinState);

    static double
    hierarchicRefinementApproximationError(
        Interaction *interaction,
        ColorRgbMutable srcRho,
        ColorRgbMutable rcvRho,
        GalerkinState *galerkinState);

    static double
    sourceClusterRadianceVariationError(
        Interaction *interaction,
        ColorRgbMutable rcvRho,
        double receiverArea,
        GalerkinState *galerkinState);

    static InteractionEvaluationCode
    hierarchicRefinementEvaluateInteraction(
        Interaction *interaction,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementComputeLightTransport(
        Interaction *interaction,
        GalerkinState *galerkinState);

    static int
    hierarchicRefinementCreateSubdivisionLink(
        const Scene *scene,
        const java::ArrayList<RpkGeometry *> *candidatesList,
        GalerkinElement *receiverElement,
        GalerkinElement *sourceElement,
        Interaction *interaction,
        const GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierarchicRefinementStoreInteraction(
        Interaction *interaction,
        const GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void regularSubDivide(GalerkinElement *element, RefinementWorkspace *workspace);

    static void
    refineOrForkSubInteraction(
        const Scene *scene,
        java::ArrayList<RpkGeometry *> *candidatesList,
        GalerkinElement *receiverElement,
        GalerkinElement *sourceElement,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    refineSubInteraction(
        const Scene *scene,
        java::ArrayList<RpkGeometry *> *candidatesList,
        GalerkinElement *receiverElement,
        GalerkinElement *sourceElement,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierarchicRefinementRegularSubdivideSource(
        const Scene *scene,
        java::ArrayList<RpkGeometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierarchicRefinementRegularSubdivideReceiver(
        const Scene *scene,
        java::ArrayList<RpkGeometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierarchicRefinementSubdivideSourceCluster(
        const Scene *scene,
        java::ArrayList<RpkGeometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierarchicRefinementSubdivideReceiverCluster(
        const Scene *scene,
        java::ArrayList<RpkGeometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static bool
    refineRecursive(
        const Scene *scene,
        java::ArrayList<RpkGeometry *> **candidatesList,
        Interaction *interaction,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static bool
    refineInteraction(
        const Scene *scene,
        Interaction *interaction,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    refineInteractionsRecursive(
        const Scene *scene,
        const GalerkinElement *parentElement,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static bool parallelRefinementSupported(const Scene *scene, const GalerkinState *galerkinState);
    static int numberOfRefinementThreads(const GalerkinState *galerkinState);

    static void
    collectRefinementItems(
        const GalerkinElement *parentElement,
        java::ArrayList<RefinementGroup> *groups,
        java::ArrayList<Interaction *> *items);

    static bool
    commitRefinementTask(
        const RefinementScheduler *scheduler,
        const RefinementTask *task,
        RefinementRecord *records,
        java::ArrayList<Interaction *> *transports,
        int *nextElementId);

    static void
    applyLightTransports(
        const java::ArrayList<Interaction *> *transports,
        GalerkinState *galerkinState,
        int numberOfThreads);

    static void receiverLightTransport(const Interaction *interaction, GalerkinState *galerkinState);
    static void sourceLightTransport(const Interaction *interaction, const GalerkinState *galerkinState);

  public:
    static void
    refineInteractions(
        const Scene *scene,
        const GalerkinElement *parentElement,
        GalerkinState *galerkinState);

    /**
    Same result as refineInteractions with Jacobi iterations, bit for bit, but
    the refinement and form factors of the interactions are computed by
    GalerkinState::numberOfThreads workers, and what they change (light
    transport, new interactions, element ids, warnings) is then committed in
    the serial order. Configurations whose refinement reads or writes shared
    scratch (non isotropic clustering, shaft culling, multi-resolution
    visibility, ray capture) are refined serially
    */
    static void
    refineInteractionsInParallel(
        const Scene *scene,
        const GalerkinElement *parentElement,
        GalerkinState *galerkinState);
};

#endif
