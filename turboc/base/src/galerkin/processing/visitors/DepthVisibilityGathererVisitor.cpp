#include "galerkin/processing/ClusterTraversalStrategy.h"
#include "galerkin/processing/visitors/DepthVisibilityGathererVisitor.h"
DepthVisibilityGathererVisitor::DepthVisibilityGathererVisitor(
    Interaction *inLink,
    ColorRgb *inSourceRadiance,
    double inPixelArea)
{
    link = inLink;
    sourceRadiance = inSourceRadiance;
    pixelArea = inPixelArea;
}

DepthVisibilityGathererVisitor::~DepthVisibilityGathererVisitor() {
}

/**
Accumulates gather contribution for a visible leaf using scratch-buffer visibility.
The visible fraction is estimated from the number of scratch pixels assigned to
the element (`scratchVisibilityUsageCounter`) times `pixelArea`, normalized by
one quarter of the receiver area. The method then calls isotropic gathering
with that area factor and clears the counter for reuse.
*/
void
DepthVisibilityGathererVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState */*galerkinState*/)
{
    if ( galerkinElement->scratchVisibilityUsageCounter <= 0 ) {
        // Element occupies no pixels in the scratch frame buffer
        return;
    }

    double areaFactor = pixelArea * ((double)(galerkinElement->scratchVisibilityUsageCounter)) / (0.25 * link->receiverElement->area);
    ClusterTraversalStrategy::isotropicGatherRadiance(galerkinElement, areaFactor, link, sourceRadiance);

    galerkinElement->scratchVisibilityUsageCounter = 0; // Set it to zero for future re-use
}
