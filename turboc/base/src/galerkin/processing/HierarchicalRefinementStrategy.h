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
    refineInteractionsRecursive(
        const Scene *scene,
        const GalerkinElement *parentElement,
        GalerkinState *galerkinState);

  public:
    static void
    refineInteractions(
        const Scene *scene,
        const GalerkinElement *parentElement,
        GalerkinState *galerkinState);
};

#endif
