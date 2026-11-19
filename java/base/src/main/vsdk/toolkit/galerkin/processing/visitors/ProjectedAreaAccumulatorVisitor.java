package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.processing.ClusterTraversalStrategy;

public final class ProjectedAreaAccumulatorVisitor implements ClusterLeafVisitor {
    private double totalProjectedArea;

    public ProjectedAreaAccumulatorVisitor() {
        totalProjectedArea = 0.0;
    }

    @Override
    public void visit(
        GalerkinElement galerkinElement,
        GalerkinState galerkinState) {
        totalProjectedArea += ClusterTraversalStrategy.surfaceProjectedAreaToSamplePoint(galerkinElement);
    }

    public double getTotalProjectedArea() {
        return totalProjectedArea;
    }
}
