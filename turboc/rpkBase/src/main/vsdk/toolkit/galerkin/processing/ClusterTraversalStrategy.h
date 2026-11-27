#ifndef __CLUSTER_TRAVERSAL_STRATEGY__
#define __CLUSTER_TRAVERSAL_STRATEGY__

#include "vsdk/toolkit/galerkin/GalerkinElement.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"

class ElementSamples;
class ScratchVisibilityWorkspace;

/**
The scratch workspace, used by the Z-visibility clustering strategy only (it
may be NULL for the others), is the solver's
*/
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

    static ColorRgbMutable sourceClusterRadiance(Interaction *link, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState);
    static double receiverArea(Interaction *link, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState);
    static void gatherRadiance(Interaction *link, ColorRgbMutable *srcRad, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState);
    static bool receiverSamples(Interaction *link, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState, double *pixelArea);
    static void gatherRadianceFromSamples(
        Interaction *link,
        ColorRgbMutable *srcRad,
        double pixelArea,
        const ElementSamples *visibleElements,
        long int numberOfVisibleElements);
    static ColorRgbMutable maxRadiance(GalerkinElement *cluster, GalerkinState *galerkinState);
    static ColorRgbMutable clusterRadianceToSamplePoint(GalerkinElement *sourceElement, Vector3Dd samplePoint, ScratchVisibilityWorkspace *scratch, GalerkinState *galerkinState);
};

#endif
