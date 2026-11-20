package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.environment.geometry.elements.RayHit;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.numericalAnalysis.CubatureRule;

/**
Scratch of the area to area form factor computation. Very often the source or
receiver element is the same as in the previous computation, so the cubature
rules, nodes and kernel values of the last element pair are kept here and
reused.

Workspace: mutable, owned by the solver (one per worker later) and passed
explicitly to FormFactorStrategy.

Java adaptation: the ray, hit record and distance cells that the C++ port keeps
on the stack live here too, so the computation allocates nothing.
*/
public class FormFactorWorkspace {
    private GalerkinElement lastReceiver;
    private GalerkinElement lastSource;
    private CubatureRule receiverCubatureRule;
    private CubatureRule sourceCubatureRule;
    private final Vector3D[] receiverNodes = new Vector3D[CubatureRule.MAXIMUM_NODES];
    private final Vector3D[] sourceNodes = new Vector3D[CubatureRule.MAXIMUM_NODES];
    private final double[][] kernel = new double[CubatureRule.MAXIMUM_NODES][CubatureRule.MAXIMUM_NODES];
    private double maximumKernelValue;
    private int visibilityCount; // Number of rays that "pass" occluders

    private final Ray kernelRay = new Ray();
    private final RayHit kernelHitStore = new RayHit();
    private final float[] kernelDistance = new float[1];
    private final float[] shadowTestDistance = new float[1];

    public FormFactorWorkspace() {
        for ( int i = 0; i < CubatureRule.MAXIMUM_NODES; i++ ) {
            receiverNodes[i] = new Vector3D();
            sourceNodes[i] = new Vector3D();
        }
    }

    GalerkinElement getLastReceiver() {
        return lastReceiver;
    }

    GalerkinElement getLastSource() {
        return lastSource;
    }

    void setLastPair(GalerkinElement receiver, GalerkinElement source) {
        lastReceiver = receiver;
        lastSource = source;
    }

    CubatureRule getReceiverCubatureRule() {
        return receiverCubatureRule;
    }

    CubatureRule getSourceCubatureRule() {
        return sourceCubatureRule;
    }

    void setReceiverCubatureRule(CubatureRule rule) {
        receiverCubatureRule = rule;
    }

    void setSourceCubatureRule(CubatureRule rule) {
        sourceCubatureRule = rule;
    }

    Vector3D[] getReceiverNodes() {
        return receiverNodes;
    }

    Vector3D[] getSourceNodes() {
        return sourceNodes;
    }

    double[][] getKernel() {
        return kernel;
    }

    double getMaximumKernelValue() {
        return maximumKernelValue;
    }

    void setMaximumKernelValue(double value) {
        maximumKernelValue = value;
    }

    int getVisibilityCount() {
        return visibilityCount;
    }

    void setVisibilityCount(int value) {
        visibilityCount = value;
    }

    Ray getKernelRay() {
        return kernelRay;
    }

    RayHit getKernelHitStore() {
        return kernelHitStore;
    }

    float[] getKernelDistance() {
        return kernelDistance;
    }

    float[] getShadowTestDistance() {
        return shadowTestDistance;
    }
}
