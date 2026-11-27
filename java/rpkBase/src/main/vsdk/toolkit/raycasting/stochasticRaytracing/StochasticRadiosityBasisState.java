package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;

public class StochasticRadiosityBasisState {
    public static final int NUMBER_OF_APPROXIMATION_TYPES = 5;

    public ApproximationTypeDescription[] approxDesc;
    public StochasticRadiosityBasis[][] basis;
    public StochasticRadiosityBasis triBasis;
    public StochasticRadiosityBasis quadBasis;
    public StochasticRadiosityBasis dummyBasis;
    public StochasticRadiosityBasis clusterBasis;
    public Matrix2x2[] quadUpTransform;
    public Matrix2x2[] triangleUpTransform;
    public boolean inited;

    static final StochasticRadiosityBasis.BasisFunction[] oneBasisTable = new StochasticRadiosityBasis.BasisFunction[] {
        Basismcrad::oneBasis
    };

    public StochasticRadiosityBasisState() {
        approxDesc = new ApproximationTypeDescription[NUMBER_OF_APPROXIMATION_TYPES];
        for ( int i = 0; i < approxDesc.length; i++ ) {
            approxDesc[i] = new ApproximationTypeDescription();
        }

        basis = new StochasticRadiosityBasis[StochasticRadiosityElementTypeInfo.NUMBER_OF_ELEMENT_TYPES][NUMBER_OF_APPROXIMATION_TYPES];

        triBasis = Basistrimcrad.createBasis();
        quadBasis = Basismcrad.stochasticRadiosityCreateQuadBasis();

        dummyBasis = new StochasticRadiosityBasis();
        dummyBasis.description = "dummy basis";
        dummyBasis.size = 0;
        dummyBasis.function = null;
        dummyBasis.dualFunction = null;
        dummyBasis.regularFilter = null;

        clusterBasis = new StochasticRadiosityBasis();
        clusterBasis.description = "cluster basis";
        clusterBasis.size = 1;
        clusterBasis.function = oneBasisTable;
        clusterBasis.dualFunction = oneBasisTable;
        clusterBasis.regularFilter = null;

        quadUpTransform = new Matrix2x2[4];
        triangleUpTransform = new Matrix2x2[4];

        approxDesc[0].name = "constant";
        approxDesc[0].basis_size = 1;
        approxDesc[1].name = "linear";
        approxDesc[1].basis_size = 3;
        approxDesc[2].name = "bilinear";
        approxDesc[2].basis_size = 4;
        approxDesc[3].name = "quadratic";
        approxDesc[3].basis_size = 6;
        approxDesc[4].name = "cubic";
        approxDesc[4].basis_size = 10;

        quadUpTransform[0] = createTransform(0.5, 0.0, 0.0, 0.5, 0.0, 0.0);
        quadUpTransform[1] = createTransform(0.5, 0.0, 0.0, 0.5, 0.5, 0.0);
        quadUpTransform[2] = createTransform(0.5, 0.0, 0.0, 0.5, 0.0, 0.5);
        quadUpTransform[3] = createTransform(0.5, 0.0, 0.0, 0.5, 0.5, 0.5);

        triangleUpTransform[0] = createTransform(0.5, 0.0, 0.0, 0.5, 0.0, 0.0);
        triangleUpTransform[1] = createTransform(0.5, 0.0, 0.0, 0.5, 0.5, 0.0);
        triangleUpTransform[2] = createTransform(0.5, 0.0, 0.0, 0.5, 0.0, 0.5);
        triangleUpTransform[3] = createTransform(-0.5, 0.0, 0.0, -0.5, 0.5, 0.5);

        inited = false;
    }

    public static void setActiveState(StochasticRadiosityBasisState state) {
        activeStatePtr = state;
    }

    public static StochasticRadiosityBasisState activeState() {
        if ( activeStatePtr == null ) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "StochasticRadiosityBasisState::activeState", "Stochastic radiosity basis state was not initialized");
        }
        return activeStatePtr;
    }

    static Matrix2x2 createTransform(double m00, double m01, double m10, double m11, double t0, double t1) {
        return new Matrix2x2(m00, m01, m10, m11, t0, t1);
    }

    private static StochasticRadiosityBasisState activeStatePtr = null;
}
