#include "GALERKIN/processing/visitors/OrientedGathererVisitor.h"
#include "GALERKIN/processing/ClusterTraversalStrategy.h"

OrientedGathererVisitor::OrientedGathererVisitor(Interaction *inLink, ColorRgb *inSourceRadiance) {
    link = inLink;
    sourceRadiance = inSourceRadiance;
}

OrientedGathererVisitor::~OrientedGathererVisitor() {
}

/**
Element is a surface element belonging to the receiver cluster
in the interaction. This routines gathers radiance to this receiver
surface, taking into account the projected area of the receiver
towards the midpoint of the source, ignoring visibility in the receiver
cluster
*/
void
OrientedGathererVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState *galerkinState,
    ColorRgb *accumulatedRadiance)
{
    // globalTheLink->rcv is a cluster, so it's total area divided by 4 (average projected area)
    // was used to compute link->K
    double areaFactor = ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(galerkinElement) /
                        (0.25 * link->receiverElement->area);

    ClusterTraversalStrategy::isotropicGatherRadiance(galerkinElement, areaFactor, link, sourceRadiance);
}
