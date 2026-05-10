package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;

public class StochasticRadiosityBasisState {
    public static final int NUMBER_OF_APPROXIMATION_TYPES = 5;

    public ApproximationTypeDescription[] approxDesc;
    public GalerkinBasis[][] basis;
    public GalerkinBasis triBasis;
    public GalerkinBasis quadBasis;
    public GalerkinBasis dummyBasis;
    public GalerkinBasis clusterBasis;
    public Matrix2x2[] quadUpTransform;
    public Matrix2x2[] triangleUpTransform;
    public boolean inited;

    static final GalerkinBasis.BasisFunction[] oneBasisTable = new GalerkinBasis.BasisFunction[] {
        Basismcrad::oneBasis
    };

    public StochasticRadiosityBasisState() {
        approxDesc = new ApproximationTypeDescription[NUMBER_OF_APPROXIMATION_TYPES];
        for ( int i = 0; i < approxDesc.length; i++ ) {
            approxDesc[i] = new ApproximationTypeDescription();
        }

        basis = new GalerkinBasis[StochasticRadiosityElementTypeInfo.NUMBER_OF_ELEMENT_TYPES][NUMBER_OF_APPROXIMATION_TYPES];

        triBasis = Basistrimcrad.createBasis();
        quadBasis = Basismcrad.stochasticRadiosityCreateQuadBasis();

        dummyBasis = new GalerkinBasis();
        dummyBasis.description = "dummy basis";
        dummyBasis.size = 0;
        dummyBasis.function = null;
        dummyBasis.dualFunction = null;
        dummyBasis.regularFilter = null;

        clusterBasis = new GalerkinBasis();
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

        quadUpTransform[0] = createTransform(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f);
        quadUpTransform[1] = createTransform(0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f);
        quadUpTransform[2] = createTransform(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f);
        quadUpTransform[3] = createTransform(0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f);

        triangleUpTransform[0] = createTransform(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f);
        triangleUpTransform[1] = createTransform(0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f);
        triangleUpTransform[2] = createTransform(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f);
        triangleUpTransform[3] = createTransform(-0.5f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f);

        inited = false;
    }

    public static void setActiveState(StochasticRadiosityBasisState state) {
        activeStatePtr = state;
    }

    public static StochasticRadiosityBasisState activeState() {
        if ( activeStatePtr == null ) {
            Logger.fatal(-1, "StochasticRadiosityBasisState::activeState", "Stochastic radiosity basis state was not initialized");
        }
        return activeStatePtr;
    }

    static Matrix2x2 createTransform(float m00, float m01, float m10, float m11, float t0, float t1) {
        Matrix2x2 transform = new Matrix2x2();
        transform.m[0][0] = m00;
        transform.m[0][1] = m01;
        transform.m[1][0] = m10;
        transform.m[1][1] = m11;
        transform.t[0] = t0;
        transform.t[1] = t1;
        return transform;
    }

    private static StochasticRadiosityBasisState activeStatePtr = null;
}
