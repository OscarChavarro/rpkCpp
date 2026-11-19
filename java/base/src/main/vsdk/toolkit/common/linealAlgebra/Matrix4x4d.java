package vsdk.toolkit.common.linealAlgebra;

/**
Transform vector v3b by m4 and put into v3a
*/

/**
Transform p3b by m4 and put into p3a
*/

// Transform as vector

// Translate

/**
Multiply m4b X m4c and put into m4a
*/

public class Matrix4x4d {
    public double[][] m;

    public Matrix4x4d() {
        m = new double[4][4];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                m[i][j] = (i == j) ? 1.0 : 0.0;
            }
        }
    }

    public void multiply(Vector3Dd v3a, Vector3Dd v3b) {
        Matrix4x4d tmp = new Matrix4x4d();

        tmp.m[0][0] = v3b.x * m[0][0] + v3b.y * m[1][0] + v3b.z * m[2][0];
        tmp.m[0][1] = v3b.x * m[0][1] + v3b.y * m[1][1] + v3b.z * m[2][1];
        tmp.m[0][2] = v3b.x * m[0][2] + v3b.y * m[1][2] + v3b.z * m[2][2];

        v3a.x = tmp.m[0][0];
        v3a.y = tmp.m[0][1];
        v3a.z = tmp.m[0][2];
    }

    public void multiplyWithTranslation(Vector3Dd p3a, Vector3Dd p3b) {
        multiply(p3a, p3b);
        p3a.x += m[3][0];
        p3a.y += m[3][1];
        p3a.z += m[3][2];
    }

    public void copy(Matrix4x4d source) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                m[i][j] = source.m[i][j];
            }
        }
    }

    public void identity() {
        Matrix4x4d tmp = new Matrix4x4d();
        copy(tmp);
    }

    public static void multiplyMatrix4(Matrix4x4d m4a, Matrix4x4d m4b, Matrix4x4d m4c) {
        Matrix4x4d tmp = new Matrix4x4d();
        for (int i = 3; i >= 0; i--) {
            for (int j = 3; j >= 0; j--) {
                tmp.m[i][j] =
                    m4b.m[i][0] * m4c.m[0][j] +
                        m4b.m[i][1] * m4c.m[1][j] +
                        m4b.m[i][2] * m4c.m[2][j] +
                        m4b.m[i][3] * m4c.m[3][j];
            }
        }

        m4a.copy(tmp);
    }
}
