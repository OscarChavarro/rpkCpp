#ifndef CLUSTER_GALERKIN__
#define CLUSTER_GALERKIN__

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"

class ClusterTraversalStrategy {
  public:
    static double surfaceProjectedAreaToSamplePoint(const GalerkinElement *receiverElement);
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
