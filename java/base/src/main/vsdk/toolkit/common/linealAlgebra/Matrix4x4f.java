package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import java.util.Arrays;
import java.util.Objects;

import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.VSDKFatalException;
import vsdk.toolkit.common.logging.Logger;

/**
Single precision 4x4 matrix: the sixteen values are floats and every
operation computes in float. Sums of products run from the first term to the
last (a0 * b0 + a1 * b1 + a2 * b2 + a3 * b3), so the results do not depend on
an accumulator starting at zero
*/
public final class Matrix4x4f extends FundamentalEntity
{
    @Serial
    private static final long serialVersionUID = 20260514L;

    private static final int SIZE = 4;

    private final float[][] m;

    public Matrix4x4f() { this(buildIdentityValues(), false); }

    public Matrix4x4f(Matrix4x4f other) {
        this(Objects.requireNonNull(other, "Matrix to copy cannot be null").m, true);
    }

    public Matrix4x4f(float[][] values) { this(values, true); }

    private Matrix4x4f(float[][] values, boolean deepCopy) {
        validate4x4(values);
        this.m = deepCopy ? deepCopy(values) : values;
    }

    public static Matrix4x4f copyOf(Matrix4x4f other) { return new Matrix4x4f(other); }
    public static Matrix4x4f copyOf(float[][] values) { return new Matrix4x4f(values); }
    public static Matrix4x4f identityMatrix() { return new Matrix4x4f(); }
    public Matrix4x4f identity() { return identityMatrix(); }

    public float get(int row, int column) { validatePosition(row, column); return m[row][column]; }

    public Matrix4x4f withVal(int row, int column, float val) {
        validatePosition(row, column);
        float[][] r = deepCopy(m);
        r[row][column] = val;
        return new Matrix4x4f(r, false);
    }

    public float[][] toArrayCopy() { return deepCopy(m); }

    public Matrix4x4f withoutTranslation() {
        return this.withVal(0, 3, 0.0f).withVal(1, 3, 0.0f).withVal(2, 3, 0.0f)
            .withVal(3, 0, 0.0f).withVal(3, 1, 0.0f).withVal(3, 2, 0.0f).withVal(3, 3, 1.0f);
    }

    public Vector3Df extractTranslation() { return new Vector3Df(get(0, 3), get(1, 3), get(2, 3)); }

    public Matrix4x4f withTranslation(Vector3Df t) {
        Objects.requireNonNull(t, "Translation vector cannot be null");
        return this.withVal(0, 3, t.x()).withVal(1, 3, t.y()).withVal(2, 3, t.z());
    }

    public Matrix4x4f orthogonalProjection(float l, float r, float d, float u, float n, float f) {
        return new Matrix4x4f(new float[][] {
            {2.0f / (r - l), 0.0f, 0.0f, -((r + l) / (r - l))},
            {0.0f, 2.0f / (u - d), 0.0f, -((u + d) / (u - d))},
            {0.0f, 0.0f, -2.0f / (f - n), -((f + n) / (f - n))},
            {0.0f, 0.0f, 0.0f, 1.0f}
        }, false);
    }

    public Matrix4x4f canonicalPerspectiveProjection() {
        return new Matrix4x4f(new float[][] {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f, 1.0f}
        }, false);
    }

    public Matrix4x4f frustumProjection(float l, float r, float d, float u, float n, float f) {
        return new Matrix4x4f(new float[][] {
            {2 * n / (r - l), 0.0f, (r + l) / (r - l), 0.0f},
            {0.0f, 2 * n / (u - d), (u + d) / (u - d), 0.0f},
            {0.0f, 0.0f, -((f + n) / (f - n)), -((2 * f * n) / (f - n))},
            {0.0f, 0.0f, -1.0f, 0.0f}
        }, false);
    }

    public Matrix4x4f translation(float x, float y, float z) {
        return new Matrix4x4f().withVal(0, 3, x).withVal(1, 3, y).withVal(2, 3, z);
    }

    public Matrix4x4f scale(float sx, float sy, float sz) {
        return new Matrix4x4f().withVal(0, 0, sx).withVal(1, 1, sy).withVal(2, 2, sz);
    }

    public Matrix4x4f scale(Vector3Df s) { return scale(s.x(), s.y(), s.z()); }
    public Matrix4x4f translation(Vector3Df t) { return translation(t.x(), t.y(), t.z()); }

    public Matrix4x4f eulerAnglesRotation(float yaw, float pitch, float roll) {
        Matrix4x4f r1 = new Matrix4x4f().axisRotation(roll, 1, 0, 0);
        Matrix4x4f r2 = new Matrix4x4f().axisRotation(pitch, 0, -1, 0);
        Matrix4x4f r3 = new Matrix4x4f().axisRotation(yaw, 0, 0, 1);
        return r3.multiply(r2.multiply(r1));
    }

    public Matrix4x4f axisRotation(float angle, Vector3Df axis) {
        return axisRotation(angle, axis.x(), axis.y(), axis.z());
    }

    public Matrix4x4f axisRotation(float angle, float x, float y, float z) {
        final float norm = (float) Math.sqrt(x * x + y * y + z * z);
        if ( Math.abs(norm) < (float) VSDK.EPSILON ) return new Matrix4x4f();
        x /= norm;
        y /= norm;
        z /= norm;
        final float s = (float) Math.sin(angle);
        final float c = (float) Math.cos(angle);
        final float t = 1.0f - c;
        return new Matrix4x4f(new float[][] {
            {t * x * x + c, t * x * y - s * z, t * x * z + s * y, 0.0f},
            {t * x * y + s * z, t * y * y + c, t * y * z - s * x, 0.0f},
            {t * x * z - s * y, t * y * z + s * x, t * z * z + c, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        }, false);
    }

    public Matrix4x4f inverse() { return invert(); }

    public Matrix4x4f invert() {
        final float d = determinant();
        if ( Math.abs(d) < (float) VSDK.EPSILON ) {
            Logger.reportMessage(this, Logger.ERROR, "invert", "Trying to invert a matrix with zero determinant");
            throw new VSDKFatalException("Trying to invert a matrix with zero determinant");
        }
        return cofactors().transpose().multiply(1.0f / d);
    }

    private static float minor3x3(float[][] sub) {
        return sub[0][0] * (sub[1][1] * sub[2][2] - sub[1][2] * sub[2][1]) -
               sub[0][1] * (sub[1][0] * sub[2][2] - sub[1][2] * sub[2][0]) +
               sub[0][2] * (sub[1][0] * sub[2][1] - sub[1][1] * sub[2][0]);
    }

    public Matrix4x4f cofactors() {
        float[][] r = new float[SIZE][SIZE];
        for ( int row = 0; row < SIZE; row++ ) {
            for ( int col = 0; col < SIZE; col++ ) {
                float[][] sub = new float[3][3];
                for ( int i = 0, si = 0; i < SIZE; i++ ) {
                    if ( i == row ) continue;
                    for ( int j = 0, sj = 0; j < SIZE; j++ ) {
                        if ( j == col ) continue;
                        sub[si][sj++] = m[i][j];
                    }
                    si++;
                }
                final float sign = ((row + col) % 2 == 0) ? 1.0f : -1.0f;
                r[row][col] = sign * minor3x3(sub);
            }
        }
        return new Matrix4x4f(r, false);
    }

    public Matrix4x4f transpose() {
        float[][] r = new float[SIZE][SIZE];
        for ( int i = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++ ) r[j][i] = m[i][j];
        }
        return new Matrix4x4f(r, false);
    }

    public Matrix4x4f multiply(float a) {
        float[][] r = deepCopy(m);
        for ( int i = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++ ) r[i][j] *= a;
        }
        return new Matrix4x4f(r, false);
    }

    /**
    Transforms a point: the product with (x, y, z, 1), divided by the
    resulting w unless w is within VSDK.EPSILON of zero
    */
    public Vector3Df multiply(Vector3Df e) {
        final float ex = e.x();
        final float ey = e.y();
        final float ez = e.z();

        final float rx = m[0][0] * ex + m[0][1] * ey + m[0][2] * ez + m[0][3];
        final float ry = m[1][0] * ex + m[1][1] * ey + m[1][2] * ez + m[1][3];
        final float rz = m[2][0] * ex + m[2][1] * ey + m[2][2] * ez + m[2][3];
        final float rw = m[3][0] * ex + m[3][1] * ey + m[3][2] * ez + m[3][3];

        if ( Math.abs(rw) < (float) VSDK.EPSILON ) {
            return new Vector3Df(rx, ry, rz);
        }
        final float invW = 1.0f / rw;
        return new Vector3Df(rx * invW, ry * invW, rz * invW);
    }

    public Vector4Df multiply(Vector4Df e) {
        final float ex = e.x();
        final float ey = e.y();
        final float ez = e.z();
        final float ew = e.w();
        return new Vector4Df(
            m[0][0] * ex + m[0][1] * ey + m[0][2] * ez + m[0][3] * ew,
            m[1][0] * ex + m[1][1] * ey + m[1][2] * ez + m[1][3] * ew,
            m[2][0] * ex + m[2][1] * ey + m[2][2] * ez + m[2][3] * ew,
            m[3][0] * ex + m[3][1] * ey + m[3][2] * ez + m[3][3] * ew
        );
    }

    public Matrix4x4f multiply(Matrix4x4f second) {
        float[][] r = new float[SIZE][SIZE];
        for ( int row = 0; row < SIZE; row++ ) {
            for ( int col = 0; col < SIZE; col++ ) {
                r[row][col] =
                    m[row][0] * second.m[0][col] +
                    m[row][1] * second.m[1][col] +
                    m[row][2] * second.m[2][col] +
                    m[row][3] * second.m[3][col];
            }
        }
        return new Matrix4x4f(r, false);
    }

    public float determinant() {
        float d = 0.0f;
        for ( int col = 0; col < SIZE; col++ ) {
            float[][] sub = new float[3][3];
            for ( int i = 1; i < SIZE; i++ ) {
                int sj = 0;
                for ( int j = 0; j < SIZE; j++ ) {
                    if ( j == col ) continue;
                    sub[i - 1][sj++] = m[i][j];
                }
            }
            d += ((col % 2) == 0 ? 1.0f : -1.0f) * m[0][col] * minor3x3(sub);
        }
        return d;
    }

    @Override
    public String toString() {
        StringBuilder msg = new StringBuilder();
        msg.append("\n------------------------------\n");
        for ( int i = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++ ) {
                msg.append(VSDK.formatDouble(m[i][j])).append(' ');
            }
            msg.append('\n');
        }
        msg.append("------------------------------\n");
        return msg.toString();
    }

    public double[] exportToDoubleArrayRowOrder() {
        double[] array = new double[16];
        for ( int i = 0, k = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++, k++ ) array[k] = m[i][j];
        }
        return array;
    }

    public float[] exportToFloatArrayRowOrder() {
        float[] array = new float[16];
        for ( int i = 0, k = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++, k++ ) array[k] = m[i][j];
        }
        return array;
    }

    public double[] exportToDoubleArrayColumnOrder() {
        double[] array = new double[16];
        for ( int j = 0, k = 0; j < SIZE; j++ ) {
            for ( int i = 0; i < SIZE; i++, k++ ) array[k] = m[i][j];
        }
        return array;
    }

    public float[] exportToFloatArrayColumnOrder() {
        float[] array = new float[16];
        for ( int j = 0, k = 0; j < SIZE; j++ ) {
            for ( int i = 0; i < SIZE; i++, k++ ) array[k] = m[i][j];
        }
        return array;
    }

    public Quaterniond exportToQuaternion() {
        final float tr = m[0][0] + m[1][1] + m[2][2];
        float qx;
        float qy;
        float qz;
        float qw;
        float[] q = {0, 0, 0, 0};
        final int[] nxt = {1, 2, 0};

        if ( tr > 0.0f ) {
            float s = (float) Math.sqrt(tr + 1.0f);
            qw = s / 2.0f;
            s = 0.5f / s;
            qx = (m[2][1] - m[1][2]) * s;
            qy = (m[0][2] - m[2][0]) * s;
            qz = (m[1][0] - m[0][1]) * s;
        }
        else {
            int i = 0;
            if ( m[1][1] > m[0][0] ) i = 1;
            if ( m[2][2] > m[i][i] ) i = 2;
            final int j = nxt[i];
            final int k = nxt[j];

            float s = (float) Math.sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.0f);
            q[i] = s * 0.5f;
            if ( s != 0.0f ) s = 0.5f / s;

            q[3] = (m[k][j] - m[j][k]) * s;
            q[j] = (m[j][i] + m[i][j]) * s;
            q[k] = (m[k][i] + m[i][k]) * s;

            qx = q[0];
            qy = q[1];
            qz = q[2];
            qw = q[3];
        }

        return new Quaterniond(new Vector3Dd(qx, qy, qz), qw);
    }

    public Matrix4x4f importFromQuaternion(Quaterniond a) {
        final float dx = (float) a.direction().x();
        final float dy = (float) a.direction().y();
        final float dz = (float) a.direction().z();
        final float w = (float) a.magnitude();
        final float x2 = dx + dx;
        final float y2 = dy + dy;
        final float z2 = dz + dz;
        final float xx = dx * x2;
        final float xy = dx * y2;
        final float xz = dx * z2;
        final float yy = dy * y2;
        final float yz = dy * z2;
        final float zz = dz * z2;
        final float sx = w * x2;
        final float sy = w * y2;
        final float sz = w * z2;

        return new Matrix4x4f(new float[][] {
            {1 - (yy + zz), xy - sz, xz + sy, 0.0f},
            {xy + sz, 1 - (xx + zz), yz - sx, 0.0f},
            {xz - sy, yz + sx, 1 - (xx + yy), 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        }, false);
    }

    public float obtainEulerYawAngle() {
        final float pitch = obtainEulerPitchAngle();
        final float epsilon = 0.0004f;
        final float halfPi = (float) (Math.PI / 2.0);

        Vector3Df dir = multiply(new Vector3Df(1, 0, 0)).withZ(0);

        if ( Math.abs(halfPi - pitch) < epsilon ) dir = multiply(new Vector3Df(0, 0, -1));
        if ( Math.abs(-halfPi - pitch) < epsilon ) dir = multiply(new Vector3Df(0, 0, 1));

        dir = dir.normalized();
        if ( dir.y() <= 0 ) return (float) Math.asin(dir.x()) - halfPi;
        return halfPi - (float) Math.asin(dir.x());
    }

    public float obtainEulerPitchAngle() {
        final Vector3Df dir = multiply(new Vector3Df(1, 0, 0)).normalized();
        return (float) (Math.PI / 2.0) - (float) Math.acos(dir.z());
    }

    public float obtainEulerRollAngle() {
        final float pitch = obtainEulerPitchAngle();
        final float yaw = obtainEulerYawAngle();

        final Matrix4x4f r3 = new Matrix4x4f().axisRotation(yaw, 0, 0, 1).invert();
        final Matrix4x4f r2 = new Matrix4x4f().axisRotation(pitch, 0, -1, 0).invert();
        Matrix4x4f r1 = r2.multiply(r3.multiply(this));

        final Quaterniond q = r1.exportToQuaternion().normalized();
        r1 = r1.importFromQuaternion(q);

        if ( r1.get(2, 1) >= 0 ) return (float) Math.acos(r1.get(1, 1));
        return -(float) Math.acos(r1.get(1, 1));
    }

    @Override
    public boolean equals(Object obj) {
        if ( this == obj ) return true;
        if ( !(obj instanceof Matrix4x4f other) ) return false;
        for ( int i = 0; i < SIZE; i++ ) {
            if ( !Arrays.equals(m[i], other.m[i]) ) return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int result = 1;
        for ( int i = 0; i < SIZE; i++ ) result = 31 * result + Arrays.hashCode(m[i]);
        return result;
    }

    public boolean epsilonEquals(Matrix4x4f other) { return epsilonEquals(other, (float) VSDK.EPSILON); }

    public boolean epsilonEquals(Matrix4x4f other, float epsilon) {
        if ( other == null ) return false;
        if ( epsilon < 0.0f ) throw new IllegalArgumentException("epsilon must be >= 0");
        for ( int i = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++ ) {
                if ( Math.abs(m[i][j] - other.m[i][j]) > epsilon ) return false;
            }
        }
        return true;
    }

    private static float[][] buildIdentityValues() {
        return new float[][] {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
    }

    private static void validate4x4(float[][] values) {
        if ( values == null || values.length != SIZE ) throw new IllegalArgumentException("Matrix must have 4 rows");
        for ( int i = 0; i < SIZE; i++ ) {
            if ( values[i] == null || values[i].length != SIZE ) {
                throw new IllegalArgumentException("Matrix row " + i + " must have 4 columns");
            }
        }
    }

    private static void validatePosition(int row, int column) {
        if ( row < 0 || row >= SIZE || column < 0 || column >= SIZE ) {
            throw new IndexOutOfBoundsException("Matrix position out of bounds: (" + row + ", " + column + ")");
        }
    }

    private static float[][] deepCopy(float[][] source) {
        float[][] copy = new float[SIZE][SIZE];
        for ( int i = 0; i < SIZE; i++ ) {
            System.arraycopy(source[i], 0, copy[i], 0, SIZE);
        }
        return copy;
    }
}
