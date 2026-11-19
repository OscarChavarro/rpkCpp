//=   decouple JOGL from the Matrix data model                              =
//=   invert that takes into account 4x4 matrices, not just the 3x3 case    =
//= References:                                                             =
//= [FOLE1992] Foley, vanDam, Feiner, Hughes. "Computer Graphics,           =
//=          principles and practice" - second edition, Addison Wesley,     =
//=          1992.                                                          =

package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import java.util.Arrays;
import java.util.Objects;

import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;

/**
This class is a data structure that represents a 4x4 matrix.
*/
public final class Matrix4x4d extends FundamentalEntity
{
    @Serial
    private static final long serialVersionUID = 20260419L;

    private static final int SIZE = 4;

    private final double[][] m;

    public Matrix4x4d()
    {
        this(buildIdentityValues(), false);
    }

    public Matrix4x4d(Matrix4x4d other)
    {
        this(Objects.requireNonNull(other, "Matrix to copy cannot be null").m,
             true);
    }

    public Matrix4x4d(double[][] values)
    {
        this(values, true);
    }

    private Matrix4x4d(double[][] values, boolean deepCopy)
    {
        validate4x4(values);
        this.m = deepCopy ? deepCopy(values) : values;
    }

    public static Matrix4x4d copyOf(Matrix4x4d other)
    {
        return new Matrix4x4d(other);
    }

    public static Matrix4x4d copyOf(double[][] values)
    {
        return new Matrix4x4d(values);
    }

    public static Matrix4x4d identityMatrix()
    {
        return new Matrix4x4d();
    }

    public Matrix4x4d identity()
    {
        return identityMatrix();
    }

    public double get(int row, int column)
    {
        validatePosition(row, column);
        return m[row][column];
    }

    public Matrix4x4d withVal(int row, int column, double val)
    {
        validatePosition(row, column);
        double[][] r = deepCopy(m);
        r[row][column] = val;
        return new Matrix4x4d(r, false);
    }

    public double[][] toArrayCopy()
    {
        return deepCopy(m);
    }

    public Matrix4x4d withoutTranslation()
    {
        return this
            .withVal(0, 3, 0.0)
            .withVal(1, 3, 0.0)
            .withVal(2, 3, 0.0)
            .withVal(3, 0, 0.0)
            .withVal(3, 1, 0.0)
            .withVal(3, 2, 0.0)
            .withVal(3, 3, 1.0);
    }

    public Vector3Dd extractTranslation()
    {
        return new Vector3Dd(get(0, 3), get(1, 3), get(2, 3));
    }

    public Matrix4x4d withTranslation(Vector3Dd t)
    {
        Objects.requireNonNull(t, "Translation vector cannot be null");
        return this
            .withVal(0, 3, t.x())
            .withVal(1, 3, t.y())
            .withVal(2, 3, t.z());
    }

    public Matrix4x4d orthogonalProjection(
        double leftPlaneDistance,
        double rightPlaneDistance,
        double downPlaneDistance,
        double upPlaneDistance,
        double nearPlaneDistance,
        double farPlaneDistance)
    {
        double tx = -((rightPlaneDistance + leftPlaneDistance) /
                     (rightPlaneDistance - leftPlaneDistance));
        double ty = -((upPlaneDistance + downPlaneDistance) /
                     (upPlaneDistance - downPlaneDistance));
        double tz = -((farPlaneDistance + nearPlaneDistance) /
                     (farPlaneDistance - nearPlaneDistance));

        return new Matrix4x4d(new double[][] {
            { 2 / (rightPlaneDistance - leftPlaneDistance), 0, 0, tx },
            { 0, 2 / (upPlaneDistance - downPlaneDistance), 0, ty },
            { 0, 0, -2 / (farPlaneDistance - nearPlaneDistance), tz },
            { 0, 0, 0, 1 }
        }, false);
    }

    public Matrix4x4d canonicalPerspectiveProjection()
    {
        return new Matrix4x4d(new double[][] {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 0, 0 },
            { 0, 0, -1, 1 }
        }, false);
    }

    public Matrix4x4d frustumProjection(
        double leftDistance,
        double rightDistance,
        double downDistance,
        double upDistance,
        double nearPlaneDistance,
        double farPlaneDistance)
    {
        double a = (rightDistance + leftDistance) /
                   (rightDistance - leftDistance);
        double b = (upDistance + downDistance) /
                   (upDistance - downDistance);
        double c = -((farPlaneDistance + nearPlaneDistance) /
                    (farPlaneDistance - nearPlaneDistance));
        double d = -((2 * farPlaneDistance * nearPlaneDistance) /
                    (farPlaneDistance - nearPlaneDistance));

        return new Matrix4x4d(new double[][] {
            { 2 * nearPlaneDistance / (rightDistance - leftDistance), 0, a, 0 },
            { 0, 2 * nearPlaneDistance / (upDistance - downDistance), b, 0 },
            { 0, 0, c, d },
            { 0, 0, -1, 0 }
        }, false);
    }

    public Matrix4x4d translation(double transx, double transy, double transz)
    {
        return new Matrix4x4d(new double[][] {
            { 1.0, 0.0, 0.0, transx },
            { 0.0, 1.0, 0.0, transy },
            { 0.0, 0.0, 1.0, transz },
            { 0.0, 0.0, 0.0, 1.0 }
        }, false);
    }

    public Matrix4x4d scale(double sx, double sy, double sz)
    {
        return new Matrix4x4d(new double[][] {
            { sx, 0.0, 0.0, 0.0 },
            { 0.0, sy, 0.0, 0.0 },
            { 0.0, 0.0, sz, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        }, false);
    }

    public Matrix4x4d scale(Vector3Dd s)
    {
        return scale(s.x(), s.y(), s.z());
    }

    public Matrix4x4d translation(Vector3Dd t)
    {
        return translation(t.x(), t.y(), t.z());
    }

    public Matrix4x4d eulerAnglesRotation(double yaw, double pitch, double roll)
    {
        Matrix4x4d r1 = new Matrix4x4d().axisRotation(roll, 1, 0, 0);
        Matrix4x4d r2 = new Matrix4x4d().axisRotation(pitch, 0, -1, 0);
        Matrix4x4d r3 = new Matrix4x4d().axisRotation(yaw, 0, 0, 1);
        return r3.multiply(r2.multiply(r1));
    }

    public Matrix4x4d axisRotation(double angle, Vector3Dd axis)
    {
        return axisRotation(angle, axis.x(), axis.y(), axis.z());
    }

    public Matrix4x4d axisRotation(double angle, double x, double y, double z)
    {
        double s = Math.sin(angle);
        double c = Math.cos(angle);

        double mag = Math.sqrt(x * x + y * y + z * z);
        if ( mag == 0.0 ) {
            return identity();
        }

        x /= mag;
        y /= mag;
        z /= mag;

        double xx = x * x;
        double yy = y * y;
        double zz = z * z;
        double xy = x * y;
        double yz = y * z;
        double zx = z * x;
        double xs = x * s;
        double ys = y * s;
        double zs = z * s;
        double oneC = 1 - c;

        return new Matrix4x4d(new double[][] {
            { (oneC * xx) + c,      (oneC * xy) - zs,    (oneC * zx) + ys,    0 },
            { (oneC * xy) + zs,     (oneC * yy) + c,     (oneC * yz) - xs,    0 },
            { (oneC * zx) - ys,     (oneC * yz) + xs,    (oneC * zz) + c,     0 },
            { 0,                    0,                   0,                   1 }
        }, false);
    }

    public Matrix4x4d inverse()
    {
        return invert();
    }

    public Matrix4x4d invert()
    {
        double a = 1.0 / determinant();
        Matrix4x4d n = cofactors().transpose();
        return n.multiply(a);
    }

    public Matrix4x4d cofactors()
    {
        double[][] r = new double[SIZE][SIZE];
        double[] minor3x3 = new double[9];

        for ( int row = 0; row < SIZE; row++ ) {
            for ( int column = 0; column < SIZE; column++ ) {
                fillMinor(minor3x3, row, column);
                double sign = ((row + column) % 2 == 0) ? 1.0 : -1.0;
                r[row][column] = sign * determinant3x3(minor3x3);
            }
        }
        return new Matrix4x4d(r, false);
    }

    public Matrix4x4d transpose()
    {
        double[][] r = new double[SIZE][SIZE];
        for ( int row = 0; row < SIZE; row++ ) {
            for ( int column = 0; column < SIZE; column++ ) {
                r[row][column] = m[column][row];
            }
        }
        return new Matrix4x4d(r, false);
    }

    public Matrix4x4d multiply(double a)
    {
        double[][] r = new double[SIZE][SIZE];
        for ( int row = 0; row < SIZE; row++ ) {
            for ( int column = 0; column < SIZE; column++ ) {
                r[row][column] = a * m[row][column];
            }
        }
        return new Matrix4x4d(r, false);
    }

    public Vector3Dd multiply(Vector3Dd e)
    {
        return new Vector3Dd(
            m[0][0] * e.x() + m[0][1] * e.y() + m[0][2] * e.z() + m[0][3],
            m[1][0] * e.x() + m[1][1] * e.y() + m[1][2] * e.z() + m[1][3],
            m[2][0] * e.x() + m[2][1] * e.y() + m[2][2] * e.z() + m[2][3]
        );
    }

    public Vector4Dd multiply(Vector4Dd e)
    {
        return new Vector4Dd(
            m[0][0] * e.x() + m[0][1] * e.y() + m[0][2] * e.z() + m[0][3] * e.w(),
            m[1][0] * e.x() + m[1][1] * e.y() + m[1][2] * e.z() + m[1][3] * e.w(),
            m[2][0] * e.x() + m[2][1] * e.y() + m[2][2] * e.z() + m[2][3] * e.w(),
            m[3][0] * e.x() + m[3][1] * e.y() + m[3][2] * e.z() + m[3][3] * e.w()
        );
    }

    public Matrix4x4d multiply(Matrix4x4d second)
    {
        double[][] r = new double[SIZE][SIZE];
        for ( int rowA = 0; rowA < SIZE; rowA++ ) {
            for ( int columnB = 0; columnB < SIZE; columnB++ ) {
                double accum = 0;
                for ( int rowB = 0; rowB < SIZE; rowB++ ) {
                    accum += m[rowA][rowB] * second.m[rowB][columnB];
                }
                r[rowA][columnB] = accum;
            }
        }
        return new Matrix4x4d(r, false);
    }

    public double determinant()
    {
        double[] minor3x3 = new double[9];
        double accum = 0;
        int row = 0;
        for ( int col = 0, sign = 1; col < SIZE; col++, sign *= -1 ) {
            fillMinor(minor3x3, row, col);
            accum += ((double)sign) * determinant3x3(minor3x3) * m[row][col];
        }
        return accum;
    }

    @Override
    public String toString()
    {
        StringBuilder msg = new StringBuilder();
        msg.append("\n------------------------------\n");
        for ( int row = 0; row < SIZE; row++ ) {
            for ( int column = 0; column < SIZE; column++ ) {
                msg.append(VSDK.formatDouble(m[row][column])).append(' ');
            }
            msg.append('\n');
        }
        msg.append("------------------------------\n");
        return msg.toString();
    }

    public double[] exportToDoubleArrayRowOrder()
    {
        double[] array = new double[16];
        for ( int i = 0, k = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++, k++ ) {
                array[k] = m[i][j];
            }
        }
        return array;
    }

    public float[] exportToFloatArrayRowOrder()
    {
        float[] array = new float[16];
        for ( int i = 0, k = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++, k++ ) {
                array[k] = (float)m[i][j];
            }
        }
        return array;
    }

    public double[] exportToDoubleArrayColumnOrder()
    {
        double[] array = new double[16];
        for ( int j = 0, k = 0; j < SIZE; j++ ) {
            for ( int i = 0; i < SIZE; i++, k++ ) {
                array[k] = m[i][j];
            }
        }
        return array;
    }

    public float[] exportToFloatArrayColumnOrder()
    {
        float[] array = new float[16];
        for ( int j = 0, k = 0; j < SIZE; j++ ) {
            for ( int i = 0; i < SIZE; i++, k++ ) {
                array[k] = (float)m[i][j];
            }
        }
        return array;
    }

    public Quaterniond exportToQuaternion()
    {
        double tr = m[0][0] + m[1][1] + m[2][2];
        double[] q = new double[4];
        double qx;
        double qy;
        double qz;
        double qw;
        int[] nxt = {1, 2, 0};

        if ( tr > 0.0 ) {
            double s = Math.sqrt(tr + 1.0);
            qw = s / 2.0;
            s = 0.5 / s;
            qx = (m[2][1] - m[1][2]) * s;
            qy = (m[0][2] - m[2][0]) * s;
            qz = (m[1][0] - m[0][1]) * s;
        }
        else {
            int i = 0;
            if ( m[1][1] > m[0][0] ) i = 1;
            if ( m[2][2] > m[i][i] ) i = 2;
            int j = nxt[i];
            int k = nxt[j];

            double s = Math.sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.0);
            q[i] = s * 0.5;
            if ( s != 0.0 ) s = 0.5 / s;

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

    public Matrix4x4d importFromQuaternion(Quaterniond a)
    {
        double x2 = a.direction().x() + a.direction().x();
        double y2 = a.direction().y() + a.direction().y();
        double z2 = a.direction().z() + a.direction().z();
        double xx = a.direction().x() * x2;
        double xy = a.direction().x() * y2;
        double xz = a.direction().x() * z2;
        double yy = a.direction().y() * y2;
        double yz = a.direction().y() * z2;
        double zz = a.direction().z() * z2;
        double sx = a.magnitude() * x2;
        double sy = a.magnitude() * y2;
        double sz = a.magnitude() * z2;

        return new Matrix4x4d(new double[][] {
            { 1 - (yy + zz),   xy - sz,        xz + sy,        0 },
            { xy + sz,         1 - (xx + zz),  yz - sx,        0 },
            { xz - sy,         yz + sx,        1 - (xx + yy),  0 },
            { 0,               0,              0,              1 }
        }, false);
    }

    public double obtainEulerYawAngle()
    {
        Vector3Dd dir = new Vector3Dd(1, 0, 0);
        double yaw;
        double pitch = obtainEulerPitchAngle();
        double epsilon = 0.0004;

        dir = multiply(dir).withZ(0);

        if ( Math.abs(Math.toRadians(90) - pitch) < epsilon ) {
            dir = multiply(new Vector3Dd(0, 0, -1));
        }
        if ( Math.abs(Math.toRadians(-90) - pitch) < epsilon ) {
            dir = multiply(new Vector3Dd(0, 0, 1));
        }

        dir = dir.normalized();
        if ( dir.y() <= 0 ) yaw = Math.asin(dir.x()) - Math.toRadians(90);
        else yaw = Math.toRadians(90) - Math.asin(dir.x());

        return yaw;
    }

    public double obtainEulerPitchAngle()
    {
        Vector3Dd dir = multiply(new Vector3Dd(1, 0, 0)).normalized();
        return Math.toRadians(90) - Math.acos(dir.z());
    }

    public double obtainEulerRollAngle()
    {
        double pitch = obtainEulerPitchAngle();
        double yaw = obtainEulerYawAngle();

        Matrix4x4d r3 = new Matrix4x4d().axisRotation(yaw, 0, 0, 1).invert();
        Matrix4x4d r2 = new Matrix4x4d().axisRotation(pitch, 0, -1, 0).invert();

        Matrix4x4d r1 = r2.multiply(r3.multiply(this));

        Quaterniond q = r1.exportToQuaternion().normalized();
        r1 = r1.importFromQuaternion(q);

        if ( r1.get(2, 1) >= 0 ) {
            return Math.acos(r1.get(1, 1));
        }
        return -Math.acos(r1.get(1, 1));
    }

    @Override
    public boolean equals(Object obj)
    {
        if ( this == obj ) return true;
        if ( !(obj instanceof Matrix4x4d other) ) return false;
        for ( int i = 0; i < SIZE; i++ ) {
            if ( !Arrays.equals(m[i], other.m[i]) ) return false;
        }
        return true;
    }

    @Override
    public int hashCode()
    {
        int result = 1;
        for ( int i = 0; i < SIZE; i++ ) {
            result = 31 * result + Arrays.hashCode(m[i]);
        }
        return result;
    }

    public boolean epsilonEquals(Matrix4x4d other)
    {
        return epsilonEquals(other, VSDK.EPSILON);
    }

    public boolean epsilonEquals(Matrix4x4d other, double epsilon)
    {
        if ( other == null ) {
            return false;
        }
        if ( epsilon < 0.0 ) {
            throw new IllegalArgumentException("epsilon must be >= 0");
        }
        for ( int i = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++ ) {
                if ( Math.abs(m[i][j] - other.m[i][j]) > epsilon ) {
                    return false;
                }
            }
        }
        return true;
    }

    private static double[][] buildIdentityValues()
    {
        return new double[][] {
            { 1.0, 0.0, 0.0, 0.0 },
            { 0.0, 1.0, 0.0, 0.0 },
            { 0.0, 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 0.0, 1.0 }
        };
    }

    private static void validate4x4(double[][] values)
    {
        if ( values == null || values.length != SIZE ) {
            throw new IllegalArgumentException("Matrix must have 4 rows");
        }
        for ( int i = 0; i < SIZE; i++ ) {
            if ( values[i] == null || values[i].length != SIZE ) {
                throw new IllegalArgumentException("Matrix row " + i + " must have 4 columns");
            }
        }
    }

    private static void validatePosition(int row, int column)
    {
        if ( row < 0 || row >= SIZE || column < 0 || column >= SIZE ) {
            throw new IndexOutOfBoundsException("Matrix position out of bounds: (" + row + ", " + column + ")");
        }
    }

    private static double[][] deepCopy(double[][] source)
    {
        double[][] copy = new double[SIZE][SIZE];
        for ( int i = 0; i < SIZE; i++ ) {
            System.arraycopy(source[i], 0, copy[i], 0, SIZE);
        }
        return copy;
    }

    private static double determinant3x3(double[] minor3x3)
    {
        return minor3x3[0] * minor3x3[4] * minor3x3[8]
             + minor3x3[3] * minor3x3[7] * minor3x3[2]
             + minor3x3[6] * minor3x3[1] * minor3x3[5]
             - minor3x3[2] * minor3x3[4] * minor3x3[6]
             - minor3x3[5] * minor3x3[7] * minor3x3[0]
             - minor3x3[8] * minor3x3[1] * minor3x3[3];
    }

    private void fillMinor(double[] minor3x3, int rowPivot, int columnPivot)
    {
        int index = 0;
        for ( int i = 0; i < SIZE; i++ ) {
            for ( int j = 0; j < SIZE; j++ ) {
                if ( i != rowPivot && j != columnPivot ) {
                    minor3x3[index] = m[i][j];
                    index++;
                }
            }
        }
    }
}
