package vsdk.toolkit.common.linealAlgebra;

import java.util.Arrays;

public class Matrix4x4 {
    public final double[][] m;

    public Matrix4x4() {
        m = new double[4][4];
        setIdentity();
    }

    public Matrix4x4(
        double a, double b, double c, double d,
        double e, double f, double g, double h,
        double i, double j, double k, double l,
        double mm, double n, double o, double p) {
        m = new double[4][4];
        m[0][0] = a;
        m[0][1] = b;
        m[0][2] = c;
        m[0][3] = d;

        m[1][0] = e;
        m[1][1] = f;
        m[1][2] = g;
        m[1][3] = h;

        m[2][0] = i;
        m[2][1] = j;
        m[2][2] = k;
        m[2][3] = l;

        m[3][0] = mm;
        m[3][1] = n;
        m[3][2] = o;
        m[3][3] = p;
    }

    public final void setIdentity() {
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                m[row][col] = row == col ? 1.0 : 0.0;
            }
        }
    }

    public static Matrix4x4 identity() {
        return new Matrix4x4();
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("Matrix4x4{\n");
        for (int row = 0; row < 4; row++) {
            sb.append("  ").append(Arrays.toString(m[row]));
            if (row < 3) {
                sb.append('\n');
            }
        }
        sb.append("\n}");
        return sb.toString();
    }
}
