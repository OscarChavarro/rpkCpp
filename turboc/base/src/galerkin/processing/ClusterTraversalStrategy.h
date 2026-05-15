#ifndef __CLUSTER_GALERKIN__
#define __CLUSTER_GALERKIN__

#include "galerkin/GalerkinElement.h"
#include "galerkin/GalerkinState.h"
#include "galerkin/processing/visitors/ClusterLeafVisitor.h"

class ClusterTraversalStrategy {
  public:
    static double srfcPrjctAreaTSmplPnt(const GalerkinElement *receiverElement);
    static void
    isotropicGatherRadiance(GalerkinElement *rcv, double areaFactor, const Interaction *link, const ColorRgbMutable *sourceRadiance);

    static void
    traverseAllLeafElements(
        ClusterLeafVisitor *leafVisitor,
        GalerkinElement *parentElement,
        GalerkinState *galerkinState);

    static ColorRgbMutable sourceClusterRadiance(Interaction *link, GalerkinState *galerkinState);
    static double receiverArea(Interaction *link, GalerkinState *galerkinState);
    static void gatherRadiance(Interaction *link, ColorRgbMutable *srcRad, GalerkinState *galerkinState);
    static ColorRgbMutable maxRadiance(GalerkinElement *cluster, GalerkinState *galerkinState);
    static ColorRgbMutable clusterRadianceToSamplePoint(GalerkinElement *sourceElement, Vector3D samplePoint, GalerkinState *galerkinState);
};

#endif
