package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.processing.ClusterTraversalStrategy;

public final class OrientedGathererVisitor implements ClusterLeafVisitor {
    private Interaction link;
    private ColorRgb[] sourceRadiance;

    public OrientedGathererVisitor(Interaction inLink, ColorRgb[] inSourceRadiance) {
        link = inLink;
        sourceRadiance = inSourceRadiance;
    }

    /**
Element is a surface element belonging to the receiver cluster
in the interaction. This routines gathers radiance to this receiver
surface, taking into account the projected area of the receiver
towards the midpoint of the source, ignoring visibility in the receiver
cluster
*/
    @Override
    public void visit(
        GalerkinElement galerkinElement,
        GalerkinState galerkinState)
    {
        double areaFactor = ClusterTraversalStrategy.surfaceProjectedAreaToSamplePoint(galerkinElement) /
                            (0.25 * link.receiverElement.area);

        ClusterTraversalStrategy.isotropicGatherRadiance(galerkinElement, areaFactor, link, sourceRadiance);
    }
}
