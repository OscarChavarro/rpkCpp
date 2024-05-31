#ifndef __HIERARCHICAL_REFINE__
#define __HIERARCHICAL_REFINE__

#include "scene/VoxelGrid.h"
#include "scene/Scene.h"
#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"

/**
Evaluates the interaction and returns a code telling whether it is accurate enough
for computing light transport, or what to do in order to reduce the
(estimated) error in the most efficient way. This is the famous oracle function
which is so crucial for efficient hierarchical refinement.

See DOC/galerkin.text
*/
enum InteractionEvaluationCode {
    ACCURATE_ENOUGH,
    REGULAR_SUBDIVIDE_SOURCE,
    REGULAR_SUBDIVIDE_RECEIVER,
    SUBDIVIDE_SOURCE_CLUSTER,
    SUBDIVIDE_RECEIVER_CLUSTER
};

/**
Shaft culling stuff for hierarchical refinement
*/
class HierarchicalRefinementStrategy {
  private:
    static void
    hierarchicRefinementCull(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        const GalerkinState *galerkinState);

    static void
    hierarchicRefinementUnCull(
        java::ArrayList<Geometry *> **candidatesList,
        const GalerkinState *galerkinState);

    static double
    hierarchicRefinementColorToError(ColorRgb rad);

    static double
    hierarchicRefinementLinkErrorThreshold(
        const Interaction *link,
        double receiverArea,
        const GalerkinState *galerkinState);

    static double
    hierarchicRefinementApproximationError(
        Interaction *link,
        ColorRgb srcRho,
        ColorRgb rcvRho,
        GalerkinState *galerkinState);

    static double
    sourceClusterRadianceVariationError(
        Interaction *link,
        ColorRgb rcvRho,
        double receiverArea,
        GalerkinState *galerkinState);

    static InteractionEvaluationCode
    hierarchicRefinementEvaluateInteraction(
        Interaction *link,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementComputeLightTransport(
        Interaction *link,
        GalerkinState *galerkinState);

    static int
    hierarchicRefinementCreateSubdivisionLink(
        const Scene *scene,
        const java::ArrayList<Geometry *> *candidatesList,
        GalerkinElement *rcv,
        GalerkinElement *src,
        Interaction *link,
        const GalerkinState *galerkinState);

    static void
    hierarchicRefinementStoreInteraction(Interaction *link, const GalerkinState *galerkinState);

    static void
    hierarchicRefinementRegularSubdivideSource(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementRegularSubdivideReceiver(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementSubdivideSourceCluster(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementSubdivideReceiverCluster(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static bool
    refineRecursive(
        const Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        GalerkinState *galerkinState);

    static bool refineInteraction(const Scene *scene, Interaction *link, GalerkinState *galerkinState);

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
