package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import vsdk.toolkit.common.FundamentalEntity;

/**
Single precision affine transform of the plane: a 2x2 block M and a
translation t, so that p' = M p + t:

|x'|   |m00  m01|   | x |   |t0|
|  | = |        | * |   | + |  |
|y'|   |m10  m11|   | y |   |t1|

Immutable; every operation computes in float
*/
public final class Matrix2x2 extends FundamentalEntity {
    @Serial
    private static final long serialVersionUID = 20261122L;

    private final float[][] m;
    private final float[] t;

    /**
    The identity: M = I, t = 0
    */
    public Matrix2x2() {
        this(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    }

    public Matrix2x2(float m00, float m01, float m10, float m11, float t0, float t1) {
        this.m = new float[][] {{m00, m01}, {m10, m11}};
        this.t = new float[] {t0, t1};
    }

    public Matrix2x2(Matrix2x2 other) {
        this(other.m[0][0], other.m[0][1], other.m[1][0], other.m[1][1], other.t[0], other.t[1]);
    }

    public float get(int row, int col) { return m[row][col]; }
    public float getTranslation(int row) { return t[row]; }

    /**
    M p + t
    */
    public Vector2Df transformPoint(Vector2Df p) {
        return new Vector2Df(
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
        int result = Float.hashCode(m[0][0] + 0.0f);
        result = 31 * result + Float.hashCode(m[0][1] + 0.0f);
        result = 31 * result + Float.hashCode(m[1][0] + 0.0f);
        result = 31 * result + Float.hashCode(m[1][1] + 0.0f);
        result = 31 * result + Float.hashCode(t[0] + 0.0f);
        result = 31 * result + Float.hashCode(t[1] + 0.0f);
        return result;
    }
}
