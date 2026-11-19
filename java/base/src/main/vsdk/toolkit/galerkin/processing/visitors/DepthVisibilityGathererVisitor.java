package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.processing.ClusterTraversalStrategy;

public final class DepthVisibilityGathererVisitor implements ClusterLeafVisitor {
    private Interaction link;
    private ColorRgb[] sourceRadiance;
    private double pixelArea;

    public DepthVisibilityGathererVisitor(
        Interaction inLink,
        ColorRgb[] inSourceRadiance,
        double inPixelArea)
    {
        link = inLink;
        sourceRadiance = inSourceRadiance;
        pixelArea = inPixelArea;
    }

    /**
Accumulates gather contribution for a visible leaf using scratch-buffer visibility.
The visible fraction is estimated from the number of scratch pixels assigned to
the element (`scratchVisibilityUsageCounter`) times `pixelArea`, normalized by
one quarter of the receiver area. The method then calls isotropic gathering
with that area factor and clears the counter for reuse.
*/
    @Override
    public void visit(
        GalerkinElement galerkinElement,
        GalerkinState galerkinState)
    {
        if ( galerkinElement.scratchVisibilityUsageCounter <= 0 ) {
            return;
        }

        double areaFactor = pixelArea * (double)galerkinElement.scratchVisibilityUsageCounter / (0.25 * link.receiverElement.area);
        ClusterTraversalStrategy.isotropicGatherRadiance(galerkinElement, areaFactor, link, sourceRadiance);

        galerkinElement.scratchVisibilityUsageCounter = 0; // Set it to zero for future re-use
    }
}
