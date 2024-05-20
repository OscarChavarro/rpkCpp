#ifndef __CLUSTER_GALERKIN__
#define __CLUSTER_GALERKIN__

#include "GALERKIN/GalerkinElement.h"
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/processing/visitors/ClusterLeafVisitor.h"

class ClusterTraversalStrategy {
  private:
    static void
    isotropicGatherRadiance(GalerkinElement *rcv, double areaFactor, const Interaction *link, const ColorRgb *sourceRadiance);

    static void
    orientedSurfaceGatherRadiance(GalerkinElement *rcv, const GalerkinState */*galerkinState*/, ColorRgb * /*accumulatedRadiance*/);

    static void
    zVisSurfaceGatherRadiance(GalerkinElement *rcv, const GalerkinState */*galerkinState*/, ColorRgb * /*accumulatedRadiance*/);

public:
    static double surfaceProjectedAreaToSamplePoint(const GalerkinElement *rcv);

    static void
    traverseAllLeafElements(
        ClusterLeafVisitor *leafVisitor,
        GalerkinElement *parentElement,
        void (*leafElementVisitCallBack)(GalerkinElement *elem, const GalerkinState *galerkinState, ColorRgb *accumulatedRadiance),
        GalerkinState *galerkinState, ColorRgb *accumulatedRadiance);

    static ColorRgb sourceClusterRadiance(Interaction *link, GalerkinState *galerkinState);
    static double receiverArea(Interaction *link, GalerkinState *galerkinState);
    static void gatherRadiance(Interaction *link, ColorRgb *srcRad, GalerkinState *galerkinState);
    static ColorRgb maxRadiance(GalerkinElement *cluster, GalerkinState *galerkinState);
    static ColorRgb clusterRadianceToSamplePoint(GalerkinElement *sourceElement, Vector3D samplePoint, GalerkinState *galerkinState);
};

#endif
