#ifndef __HIERARCHICAL_REFINE__
#define __HIERARCHICAL_REFINE__

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
    static void
    prepareRefinementIterationState(GalerkinState *galerkinState);

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
    hierRefClrTErrr(ColorRgbMutable radiance);

    static double
    hierRefLinkErrrThrsh(
        const Interaction *interaction,
        double receiverArea,
        const GalerkinState *galerkinState);

    static double
    hierRefApprxErrr(
        Interaction *interaction,
        ColorRgbMutable srcRho,
        ColorRgbMutable rcvRho,
        GalerkinState *galerkinState);

    static double
    srcClustRadnVrtnErrr(
        Interaction *interaction,
        ColorRgbMutable rcvRho,
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
        const GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierRefStoreInter(Interaction *interaction, const GalerkinState *galerkinState);

    static void
    refineSubInteraction(
        const Scene *scene,
        ArrayList<Geometry *> *candidatesList,
        GalerkinElement *receiverElement,
        GalerkinElement *sourceElement,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierRefRegSbdvdSrc(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierRefRegSbdvdRecv(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierRefSbdvdSrcClust(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static void
    hierRefSbdvdRecvClust(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
        Interaction *interaction,
        bool isClusteredGeometry,
        GalerkinState *galerkinState,
        RefinementWorkspace *workspace);

    static bool
    refineRecursive(
        const Scene *scene,
        ArrayList<Geometry *> **candidatesList,
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

    static void receiverLightTransport(const Interaction *interaction, GalerkinState *galerkinState);
    static void sourceLightTransport(const Interaction *interaction, const GalerkinState *galerkinState);

  public:
    static void
    refineInteractions(
        const Scene *scene,
        const GalerkinElement *parentElement,
        GalerkinState *galerkinState);
};

#endif
