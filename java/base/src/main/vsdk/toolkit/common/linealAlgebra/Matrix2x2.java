package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import vsdk.toolkit.common.FundamentalEntity;

/**
Double precision affine transform of the plane: a 2x2 block M and a
translation t, so that p' = M p + t:

|x'|   |m00  m01|   | x |   |t0|
|  | = |        | * |   | + |  |
|y'|   |m10  m11|   | y |   |t1|

Immutable; every operation computes in double
*/
public final class Matrix2x2 extends FundamentalEntity {
    @Serial
    private static final long serialVersionUID = 20261122L;

    private final double[][] m;
    private final double[] t;

    /**
    The identity: M = I, t = 0
    */
    public Matrix2x2() {
        this(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    }

    public Matrix2x2(double m00, double m01, double m10, double m11, double t0, double t1) {
        this.m = new double[][] {{m00, m01}, {m10, m11}};
        this.t = new double[] {t0, t1};
    }

    public Matrix2x2(Matrix2x2 other) {
        this(other.m[0][0], other.m[0][1], other.m[1][0], other.m[1][1], other.t[0], other.t[1]);
    }

    public double get(int row, int col) { return m[row][col]; }
    public double getTranslation(int row) { return t[row]; }

    /**
    M p + t
    */
    public Vector2Dd transformPoint(Vector2Dd p) {
        return new Vector2Dd(
            m[0][0] * p.x() + m[0][1] * p.y() + t[0],
            m[1][0] * p.x() + m[1][1] * p.y() + t[1]);
    }

    /**
    The composition this(other(p)): M = this.M * other.M and
    t = this.M * other.t + this.t
    */
    public Matrix2x2 multiply(Matrix2x2 other) {
        return new Matrix2x2(
            m[0][0] * other.m[0][0] + m[0][1] * other.m[1][0],
            m[0][0] * other.m[0][1] + m[0][1] * other.m[1][1],
            m[1][0] * other.m[0][0] + m[1][1] * other.m[1][0],
            m[1][0] * other.m[0][1] + m[1][1] * other.m[1][1],
            m[0][0] * other.t[0] + m[0][1] * other.t[1] + t[0],
            m[1][0] * other.t[0] + m[1][1] * other.t[1] + t[1]);
    }

    @Override
    public boolean equals(Object obj) {
        if ( this == obj ) {
            return true;
        }
        if ( !(obj instanceof Matrix2x2 other) ) {
            return false;
        }
        return m[0][0] == other.m[0][0] && m[0][1] == other.m[0][1] &&
            m[1][0] == other.m[1][0] && m[1][1] == other.m[1][1] &&
            t[0] == other.t[0] && t[1] == other.t[1];
    }

    @Override
    public int hashCode() {
        int result = Double.hashCode(m[0][0] + 0.0);
        result = 31 * result + Double.hashCode(m[0][1] + 0.0);
        result = 31 * result + Double.hashCode(m[1][0] + 0.0);
        result = 31 * result + Double.hashCode(m[1][1] + 0.0);
        result = 31 * result + Double.hashCode(t[0] + 0.0);
        result = 31 * result + Double.hashCode(t[1] + 0.0);
        return result;
    }
}
