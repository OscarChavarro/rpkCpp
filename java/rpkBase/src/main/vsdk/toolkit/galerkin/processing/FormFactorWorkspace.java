package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.common.statistics.ShadowStatistics;
import vsdk.toolkit.environment.geometry.elements.PatchIntersectionWorkspace;
import vsdk.toolkit.environment.geometry.elements.RayHit;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.numericalAnalysis.CubatureRule;

/**
Scratch of the area to area form factor computation. Very often the source or
receiver element is the same as in the previous computation, so the cubature
rules, nodes and kernel values of the last element pair are kept here and
reused.

Workspace: mutable, owned by the solver (the serial one) or by a parallel
Jacobi worker, and passed explicitly to FormFactorStrategy.
It also tells where the shadow rays are counted (the global statistics, or the
counters of a worker, added to them afterwards) and whether the "nodes too
close" warnings are reported at once or kept, as element pairs, for a later
report in the serial order.

Java adaptation: the ray, hit record and distance cells that the C++ port keeps
on the stack live here too, so the computation allocates nothing, and so does
the scratch of the patch intersections of the shadow cache tests.
*/
public class FormFactorWorkspace {
    private GalerkinElement lastReceiver;
    private GalerkinElement lastSource;
    private CubatureRule receiverCubatureRule;
    private CubatureRule sourceCubatureRule;
    private final Vector3Df[] receiverNodes = new Vector3Df[CubatureRule.MAXIMUM_NODES];
    private final Vector3Df[] sourceNodes = new Vector3Df[CubatureRule.MAXIMUM_NODES];
    private final double[][] kernel = new double[CubatureRule.MAXIMUM_NODES][CubatureRule.MAXIMUM_NODES];
    private double maximumKernelValue;
    private int visibilityCount; // Number of rays that "pass" occluders
    private final ShadowStatistics shadowStatistics; // Not owned
    private ArrayList<GalerkinElement> deferredNodesTooClose; // Not owned; null to report at once

    private final Ray kernelRay = new Ray();
    private final RayHit kernelHitStore = new RayHit();
    private final float[] kernelDistance = new float[1];
    private final float[] shadowTestDistance = new float[1];
    private final PatchIntersectionWorkspace patchWorkspace = new PatchIntersectionWorkspace();

    public FormFactorWorkspace(ShadowStatistics inShadowStatistics) {
        shadowStatistics = inShadowStatistics;
        deferredNodesTooClose = null;
        for ( int i = 0; i < CubatureRule.MAXIMUM_NODES; i++ ) {
            receiverNodes[i] = new Vector3Df();
            sourceNodes[i] = new Vector3Df();
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

    Vector3Df[] getReceiverNodes() {
        return receiverNodes;
    }

    Vector3Df[] getSourceNodes() {
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

    PatchIntersectionWorkspace getPatchWorkspace() {
        return patchWorkspace;
    }

    ShadowStatistics getShadowStatistics() {
        return shadowStatistics;
    }

    /**
    @param pairs list receiving (receiver, source) pairs, or null to report the warning at once
    */
    void setDeferredNodesTooClose(ArrayList<GalerkinElement> pairs) {
        deferredNodesTooClose = pairs;
    }

    /**
    @return true when the warning was kept for a later report, false when it is to be reported now
    */
    boolean deferNodesTooClose(GalerkinElement receiver, GalerkinElement source) {
        if ( deferredNodesTooClose == null ) {
            return false;
        }
        deferredNodesTooClose.add(receiver);
        deferredNodesTooClose.add(source);
        return true;
    }
}
