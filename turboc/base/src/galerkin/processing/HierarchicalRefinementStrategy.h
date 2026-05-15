#ifndef __HIERARCHICAL_REFINE__
#define __HIERARCHICAL_REFINE__

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
    static void
    hierarchicRefinementCull(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        const GalerkinState *galerkinState);

    static void
    hierarchicRefinementUnCull(
        ArrayList<Geometry *> **candidatesList,
        const GalerkinState *galerkinState);

    static double
    hierRefClrTErrr(ColorRgb radiance);

    static double
    hierRefLinkErrrThrsh(
        const Interaction *interaction,
        double receiverArea,
        const GalerkinState *galerkinState);

    static double
    hierRefApprxErrr(
        Interaction *interaction,
        ColorRgb srcRho,
        ColorRgb rcvRho,
        GalerkinState *galerkinState);

    static double
    srcClustRadnVrtnErrr(
        Interaction *interaction,
        ColorRgb rcvRho,
        double receiverArea,
        GalerkinState *galerkinState);

    static InteractionEvaluationCode
    hierRefEvalInter(
        Interaction *interaction,
        GalerkinState *galerkinState);

    static void
    hierRefCompLightTransp(
        Interaction *interaction,
        GalerkinState *galerkinState);

    static int
    hierRefCreateSubdivLink(
        const Scene *scene,
        const ArrayList<Geometry *> *candidatesList,
        GalerkinElement *receiverElement,
        GalerkinElement *sourceElement,
        Interaction *interaction,
        const GalerkinState *galerkinState);

    static void
    hierRefStoreInter(Interaction *interaction, const GalerkinState *galerkinState);

    static void
    hierRefRegSbdvdSrc(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    hierRefRegSbdvdRecv(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    hierRefSbdvdSrcClust(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static void
    hierRefSbdvdRecvClust(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState);

    static bool
    refineRecursive(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        GalerkinState *galerkinState);

    static bool refineInteraction(const Scene *scene, Interaction *interaction, GalerkinState *galerkinState);

    static void
    removeRefinedInteractions(const GalerkinState *galerkinState, const ArrayList<Interaction *> *interactionsToRemove);

  public:
    static void
    refineInteractions(
        const Scene *scene,
        const GalerkinElement *parentElement,
        GalerkinState *galerkinState);
};

#endif
