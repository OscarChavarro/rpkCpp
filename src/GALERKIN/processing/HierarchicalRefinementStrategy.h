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
enum INTERACTION_EVALUATION_CODE {
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
        Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        GalerkinState *state);

    static void
    hierarchicRefinementUnCull(
        java::ArrayList<Geometry *> **candidatesList,
        GalerkinState *state);

    static double
    hierarchicRefinementColorToError(ColorRgb rad);

    static double
    hierarchicRefinementLinkErrorThreshold(
        Interaction *link,
        double rcv_area,
        GalerkinState *state);

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
        double rcv_area,
        GalerkinState *galerkinState);

    static INTERACTION_EVALUATION_CODE
    hierarchicRefinementEvaluateInteraction(
        Interaction *link,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementComputeLightTransport(
        Interaction *link,
        GalerkinState *galerkinState);

    static int
    hierarchicRefinementCreateSubdivisionLink(
        Scene *scene,
        java::ArrayList<Geometry *> *candidatesList,
        GalerkinElement *rcv,
        GalerkinElement *src,
        Interaction *link,
        GalerkinState *galerkinState);

    static void
    hierarchicRefinementStoreInteraction(Interaction *link, GalerkinState *state);

    static void
    hierarchicRefinementRegularSubdivideSource(
        Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RenderOptions *renderOptions);

    static void
    hierarchicRefinementRegularSubdivideReceiver(
        Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RenderOptions *renderOptions);

    static void
    hierarchicRefinementSubdivideSourceCluster(
        Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RenderOptions *renderOptions);

    static void
    hierarchicRefinementSubdivideReceiverCluster(
        Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RenderOptions *renderOptions);

    static bool
    refineRecursive(
        Scene *scene,
        java::ArrayList<Geometry *> **candidatesList,
        Interaction *link,
        GalerkinState *state,
        RenderOptions *renderOptions);

    static bool
    refineInteraction(Scene *scene, Interaction *link, GalerkinState *state, RenderOptions *renderOptions);

    static void
    removeRefinedInteractions(const GalerkinState *state, java::ArrayList<Interaction *> *interactionsToRemove);

  public:
    static void
    refineInteractions(
        Scene *scene,
        GalerkinElement *parentElement,
        GalerkinState *state,
        RenderOptions *renderOptions);
};

#endif
