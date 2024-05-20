#include "GALERKIN/processing/visitors/DepthVisibilityGathererVisitor.h"
#include "GALERKIN/processing/ClusterTraversalStrategy.h"

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
Same as above, except that the number of pixels in the scratch frame buffer
times the area corresponding one such pixel is used as the visible area
of the element. Uses global variables globalPixelArea, globalTheLink, globalPSourceRad
*/
void
DepthVisibilityGathererVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState *galerkinState)
{
    if ( galerkinElement->scratchVisibilityUsageCounter <= 0 ) {
        // Element occupies no pixels in the scratch frame buffer
        return;
    }

    double areaFactor = pixelArea * (double) (galerkinElement->scratchVisibilityUsageCounter) / (0.25 * link->receiverElement->area);
    ClusterTraversalStrategy::isotropicGatherRadiance(galerkinElement, areaFactor, link, sourceRadiance);

    galerkinElement->scratchVisibilityUsageCounter = 0; // Set it to zero for future re-use
}
