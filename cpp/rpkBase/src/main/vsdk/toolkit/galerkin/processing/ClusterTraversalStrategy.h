#ifndef __CLUSTER_TRAVERSAL_STRATEGY__
#define __CLUSTER_TRAVERSAL_STRATEGY__

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"

class ElementSamples;
class ScratchVisibilityWorkspace;

/**
The scratch workspace, used by the Z-visibility clustering strategy only (it
may be nullptr for the others), is the one of the calling thread
*/
class ClusterTraversalStrategy {
  public:
    static double surfaceProjectedAreaToSamplePoint(const GalerkinElement *receiverElement);
    static void
    isotropicGatherRadiance(GalerkinElement *rcv, double areaFactor, const Interaction *link, const ColorRgb *sourceRadiance);

    static void
    traverseAllLeafElements(
        ClusterLeafVisitor *leafVisitor,
        GalerkinElement *parentElement,
        GalerkinState *galerkinState);

    static ColorRgb sourceClusterRadiance(Interaction *link, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState);
    static double receiverArea(Interaction *link, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState);
    static void gatherRadiance(Interaction *link, ColorRgb *srcRad, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState);
    static bool receiverSamples(Interaction *link, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState, double *pixelArea);
    static void gatherRadianceFromSamples(
        Interaction *link,
        ColorRgb *srcRad,
        double pixelArea,
        const ElementSamples *visibleElements,
        long int numberOfVisibleElements);
    static ColorRgb maxRadiance(GalerkinElement *cluster, GalerkinState *galerkinState);
    static ColorRgb clusterRadianceToSamplePoint(GalerkinElement *sourceElement, Vector3Dd samplePoint, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState);
};

#endif
