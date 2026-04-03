package vsdk.toolkit.common.linealAlgebra;

/**
Jacobian for a quadrilateral patch is J(u, v) = A + B.u + C.v
*/

public class Jacobian {
    public float A;
    public float B;
    public float C;

    public Jacobian(float inA, float inB, float inC) {
        A = inA;
        B = inB;
        C = inC;
    }
}
