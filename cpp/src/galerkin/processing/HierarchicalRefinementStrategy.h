#ifndef HIERARCHICAL_REFINE__
#define HIERARCHICAL_REFINE__

#include "common/memoryManagement/MemoryPool.h"
#include "scene/Scene.h"
#include "scene/VoxelGrid.h"
#include "galerkin/GalerkinElement.h"
#include "galerkin/GalerkinState.h"
#include "galerkin/processing/InteractionEvaluationCode.h"

/**
Shaft culling stuff for hierarchical refinement
*/
class HierarchicalRefinementStrategy {
  private:
    static MemoryPool<float> hierarchicalCoefficientsPool;
    static bool hierarchicalCoefficientsPoolInitialized;

    static void ensureHierarchicalCoefficientsPool();

    static void
    hierarchicRefinementCull(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        const GalerkinState *galerkinState);

    static void
    hierarchicRefinementUnCull(
        java::ArrayList<Geometry *> **candidatesList,
        const GalerkinState *galerkinState);

    static double
    hierarchicRefinementColorToError(ColorRgb radiance);

    static double
    hierarchicRefinementLinkErrorThreshold(
        const Interaction *interaction,
        double receiverArea,
        const GalerkinState *galerkinState);

    static double
    hierarchicRefinementApproximationError(
        Interaction *interaction,
        ColorRgb srcRho,
        ColorRgb rcvRho,
        GalerkinState *galerkinState);

    static double
    sourceClusterRadianceVariationError(
        Interaction *interaction,
        ColorRgb rcvRho,
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
        const java::ArrayList<Geometry *> *candidatesList,
        GalerkinElement *receiverElement,
        GalerkinElement *sourceElement,
        Interaction *interaction,
        const GalerkinState *galerkinState);

    static void
    hierarchicRefinementStoreInteraction(Interaction *interaction, const GalerkinState *galerkinState);

    static void
    hierarchicRefinementRegularSubdivideSource(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementRegularSubdivideReceiver(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementSubdivideSourceCluster(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementSubdivideReceiverCluster(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static bool
    refineRecursive(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        GalerkinState *galerkinState);

    static bool refineInteraction(const Scene *scene, Interaction *interaction, GalerkinState *galerkinState);

    static void
    removeRefinedInteractions(const GalerkinState *galerkinState, const java::ArrayList<Interaction *> *interactionsToRemove);

  public:
    static void
    refineInteractions(
        const Scene *scene,
        const GalerkinElement *parentElement,
        GalerkinState *galerkinState);
};

#endif
