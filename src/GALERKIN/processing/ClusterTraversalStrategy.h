#ifndef __CLUSTER_GALERKIN__
#define __CLUSTER_GALERKIN__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"

class ClusterTraversalStrategy {
  private:
    static void
    accumulatePowerToSamplePoint(GalerkinElement *src, GalerkinState *galerkinState, ColorRgb * /*accumulatedRadiance*/);

    static double
    surfaceProjectedAreaToSamplePoint(const GalerkinElement *rcv);

    static void
    accumulateProjectedAreaToSamplePoint(GalerkinElement *rcv, GalerkinState * /*galerkinState*/, ColorRgb * /*accumulatedRadiance*/);

    static void
    isotropicGatherRadiance(GalerkinElement *rcv, double areaFactor, Interaction *link, ColorRgb *srcRad);

    static void
    orientedSurfaceGatherRadiance(GalerkinElement *rcv, GalerkinState */*galerkinState*/, ColorRgb * /*accumulatedRadiance*/);

    static void
    zVisSurfaceGatherRadiance(GalerkinElement *rcv, GalerkinState */*galerkinState*/, ColorRgb * /*accumulatedRadiance*/);

    static void
    leafMaxRadiance(GalerkinElement *galerkinElement, GalerkinState *galerkinState, ColorRgb *accumulatedRadiance);

public:
    static void
    traverseAllLeafElements(
        GalerkinElement *parentElement,
        void (*leafElementVisitCallBack)(GalerkinElement *elem, GalerkinState *galerkinState, ColorRgb *accumulatedRadiance),
        GalerkinState *galerkinState, ColorRgb *accumulatedRadiance);

    static ColorRgb sourceClusterRadiance(Interaction *link, GalerkinState *galerkinState);
    static double receiverArea(Interaction *link, GalerkinState *galerkinState);
    static void gatherRadiance(Interaction *link, ColorRgb *srcRad, GalerkinState *galerkinState);
    static ColorRgb maxRadiance(GalerkinElement *cluster, GalerkinState *galerkinState);
    static ColorRgb clusterRadianceToSamplePoint(GalerkinElement *src, Vector3D sample, GalerkinState *galerkinState);
};

#endif
