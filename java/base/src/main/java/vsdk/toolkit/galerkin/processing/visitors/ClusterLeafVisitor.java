package vsdk.toolkit.galerkin.processing.visitors;

import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;

public interface ClusterLeafVisitor {
    void visit(GalerkinElement galerkinElement, GalerkinState galerkinState);
}
