package vsdk.toolkit.common.linealAlgebra;

import java.util.Arrays;

public class Matrix4x4 {
    public float[][] m;

    public Matrix4x4() {
        m = new float[4][4];
        m[0][0] = 1.0f;
        m[0][1] = 0.0f;
        m[0][2] = 0.0f;
        m[0][3] = 0.0f;
        m[1][0] = 0.0f;
        m[1][1] = 1.0f;
        m[1][2] = 0.0f;
        m[1][3] = 0.0f;
        m[2][0] = 0.0f;
        m[2][1] = 0.0f;
        m[2][2] = 1.0f;
        m[2][3] = 0.0f;
        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = 0.0f;
        m[3][3] = 1.0f;
    }

    public Matrix4x4(
        float a,
        float b,
        float c,
        float d,
        float e,
        float f,
        float g,
        float h,
        float i,
        float j,
        float k,
        float l,
        float mm,
        float n,
        float o,
        float p) {
        m = new float[4][4];
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

    public static Matrix4x4 identity() {
        return new Matrix4x4();
    }

    public void set3X3Matrix(
        float a,
        float b,
        float c,
        float d,
        float e,
        float f,
        float g,
        float h,
        float i) {
        m[0][0] = a;
        m[0][1] = b;
        m[0][2] = c;
        m[1][0] = d;
        m[1][1] = e;
        m[1][2] = f;
        m[2][0] = g;
        m[2][1] = h;
        m[2][2] = i;
    }

    public void transformPoint3D(Vector3D src, Vector3D dst) {
        dst.x = m[0][0] * src.x + m[0][1] * src.y + m[0][2] * src.z;
        dst.y = m[1][0] * src.x + m[1][1] * src.y + m[1][2] * src.z;
        dst.z = m[2][0] * src.x + m[2][1] * src.y + m[2][2] * src.z;
    }

    public void transformPoint4D(Vector4D src, Vector4D dst) {
        dst.x = m[0][0] * src.x + m[0][1] * src.y + m[0][2] * src.z + m[0][3] * src.w;
        dst.y = m[1][0] * src.x + m[1][1] * src.y + m[1][2] * src.z + m[1][3] * src.w;
        dst.z = m[2][0] * src.x + m[2][1] * src.y + m[2][2] * src.z + m[2][3] * src.w;
        dst.w = m[3][0] * src.x + m[3][1] * src.y + m[3][2] * src.z + m[3][3] * src.w;
    }

    public void recoverRotationParameters(float[] angle, Vector3D axis) {
        if (angle == null || angle.length == 0 || axis == null) {
            throw new IllegalArgumentException("angle and axis output parameters are required");
        }

        float c = (m[0][0] + m[1][1] + m[2][2] - 1.0f) * 0.5f;
        if (c > 1.0f - Numeric.EPSILON) {
            angle[0] = 0.0f;
            axis.set(0.0f, 0.0f, 1.0f);
        }
        else if (c < -1.0f + Numeric.EPSILON) {
            angle[0] = (float)Math.PI;
            axis.x = (float)Math.sqrt((m[0][0] + 1.0f) * 0.5f);
            axis.y = (float)Math.sqrt((m[1][1] + 1.0f) * 0.5f);
            axis.z = (float)Math.sqrt((m[2][2] + 1.0f) * 0.5f);

            if (m[1][0] < 0.0f) {
                axis.y = -axis.y;
            }
            if (m[2][0] < 0.0f) {
                axis.z = -axis.z;
            }
        }
        else {
            angle[0] = (float)Math.acos(c);
            float s = (float)Math.sqrt(1.0f - c * c);
            float r = 1.0f / (2.0f * s);
            axis.x = (m[2][1] - m[1][2]) * r;
            axis.y = (m[0][2] - m[2][0]) * r;
            axis.z = (m[1][0] - m[0][1]) * r;
        }
    }

    public static Matrix4x4 createTransComposeMatrix(Matrix4x4 xf2, Matrix4x4 xf1) {
        Matrix4x4 xf = new Matrix4x4();

        xf.m[0][0] = xf2.m[0][0] * xf1.m[0][0] + xf2.m[0][1] * xf1.m[1][0] + xf2.m[0][2] * xf1.m[2][0] + xf2.m[0][3] * xf1.m[3][0];
        xf.m[0][1] = xf2.m[0][0] * xf1.m[0][1] + xf2.m[0][1] * xf1.m[1][1] + xf2.m[0][2] * xf1.m[2][1] + xf2.m[0][3] * xf1.m[3][1];
        xf.m[0][2] = xf2.m[0][0] * xf1.m[0][2] + xf2.m[0][1] * xf1.m[1][2] + xf2.m[0][2] * xf1.m[2][2] + xf2.m[0][3] * xf1.m[3][2];
        xf.m[0][3] = xf2.m[0][0] * xf1.m[0][3] + xf2.m[0][1] * xf1.m[1][3] + xf2.m[0][2] * xf1.m[2][3] + xf2.m[0][3] * xf1.m[3][3];

        xf.m[1][0] = xf2.m[1][0] * xf1.m[0][0] + xf2.m[1][1] * xf1.m[1][0] + xf2.m[1][2] * xf1.m[2][0] + xf2.m[1][3] * xf1.m[3][0];
        xf.m[1][1] = xf2.m[1][0] * xf1.m[0][1] + xf2.m[1][1] * xf1.m[1][1] + xf2.m[1][2] * xf1.m[2][1] + xf2.m[1][3] * xf1.m[3][1];
        xf.m[1][2] = xf2.m[1][0] * xf1.m[0][2] + xf2.m[1][1] * xf1.m[1][2] + xf2.m[1][2] * xf1.m[2][2] + xf2.m[1][3] * xf1.m[3][2];
        xf.m[1][3] = xf2.m[1][0] * xf1.m[0][3] + xf2.m[1][1] * xf1.m[1][3] + xf2.m[1][2] * xf1.m[2][3] + xf2.m[1][3] * xf1.m[3][3];

        xf.m[2][0] = xf2.m[2][0] * xf1.m[0][0] + xf2.m[2][1] * xf1.m[1][0] + xf2.m[2][2] * xf1.m[2][0] + xf2.m[2][3] * xf1.m[3][0];
        xf.m[2][1] = xf2.m[2][0] * xf1.m[0][1] + xf2.m[2][1] * xf1.m[1][1] + xf2.m[2][2] * xf1.m[2][1] + xf2.m[2][3] * xf1.m[3][1];
        xf.m[2][2] = xf2.m[2][0] * xf1.m[0][2] + xf2.m[2][1] * xf1.m[1][2] + xf2.m[2][2] * xf1.m[2][2] + xf2.m[2][3] * xf1.m[3][2];
        xf.m[2][3] = xf2.m[2][0] * xf1.m[0][3] + xf2.m[2][1] * xf1.m[1][3] + xf2.m[2][2] * xf1.m[2][3] + xf2.m[2][3] * xf1.m[3][3];

        xf.m[3][0] = xf2.m[3][0] * xf1.m[0][0] + xf2.m[3][1] * xf1.m[1][0] + xf2.m[3][2] * xf1.m[2][0] + xf2.m[3][3] * xf1.m[3][0];
        xf.m[3][1] = xf2.m[3][0] * xf1.m[0][1] + xf2.m[3][1] * xf1.m[1][1] + xf2.m[3][2] * xf1.m[2][1] + xf2.m[3][3] * xf1.m[3][1];
        xf.m[3][2] = xf2.m[3][0] * xf1.m[0][2] + xf2.m[3][1] * xf1.m[1][2] + xf2.m[3][2] * xf1.m[2][2] + xf2.m[3][3] * xf1.m[3][2];
        xf.m[3][3] = xf2.m[3][0] * xf1.m[0][3] + xf2.m[3][1] * xf1.m[1][3] + xf2.m[3][2] * xf1.m[2][3] + xf2.m[3][3] * xf1.m[3][3];

        return xf;
    }

    public static Matrix4x4 createTranslationMatrix(Vector3D translation) {
        Matrix4x4 xf = new Matrix4x4();
        xf.m[0][3] = translation.x;
        xf.m[1][3] = translation.y;
        xf.m[2][3] = translation.z;
        return xf;
    }

    public static Matrix4x4 createPerspectiveMatrix(float fieldOfViewInRadians, float aspect, float near, float far) {
        Matrix4x4 xf = new Matrix4x4();
        float f = 1.0f / (float)Math.tan(fieldOfViewInRadians / 2.0f);

        xf.m[0][0] = f / aspect;
        xf.m[1][1] = f;
        xf.m[2][2] = (near + far) / (near - far);
        xf.m[2][3] = (2 * far * near) / (near - far);
        xf.m[3][2] = -1.0f;
        xf.m[3][3] = 0.0f;

        return xf;
    }

    public static Matrix4x4 createRotationMatrix(float angleInRadians, Vector3D axis) {
        Matrix4x4 xf = new Matrix4x4();

        float s = axis.norm();
        if (s < Numeric.EPSILON) {
            return xf;
        }

        axis.inverseScaledCopy(s, axis, Numeric.EPSILON_FLOAT);

        float x = axis.x;
        float y = axis.y;
        float z = axis.z;
        float c = (float)Math.cos(angleInRadians);
        s = (float)Math.sin(angleInRadians);
        float t = 1.0f - c;

        xf.set3X3Matrix(
            x * x * t + c, x * y * t - z * s, x * z * t + y * s,
            x * y * t + z * s, y * y * t + c, y * z * t - x * s,
            x * z * t - y * s, y * z * t + x * s, z * z * t + c);

        return xf;
    }

    public static Matrix4x4 createLookAtMatrix(Vector3D eye, Vector3D centre, Vector3D up) {
        Matrix4x4 xf = new Matrix4x4();
        Vector3D s = new Vector3D();
        Vector3D xAxis = new Vector3D();
        Vector3D yAxis = new Vector3D();
        Vector3D zAxis = new Vector3D();

        zAxis.subtraction(eye, centre);
        zAxis.normalize(Numeric.EPSILON_FLOAT);

        xAxis.crossProduct(up, zAxis);
        xAxis.normalize(Numeric.EPSILON_FLOAT);

        yAxis.crossProduct(zAxis, xAxis);
        xf.set3X3Matrix(
            xAxis.x, xAxis.y, xAxis.z,
            yAxis.x, yAxis.y, yAxis.z,
            zAxis.x, zAxis.y, zAxis.z);

        s.scaledCopy(-1.0f, eye);
        Matrix4x4 t = createTranslationMatrix(s);
        return createTransComposeMatrix(xf, t);
    }

    public static Matrix4x4 createOrthogonalViewMatrix(float left, float right, float bottom, float top, float near, float far) {
        Matrix4x4 xf = new Matrix4x4();

        xf.m[0][0] = 2.0f / (right - left);
        xf.m[0][3] = -(right + left) / (right - left);

        xf.m[1][1] = 2.0f / (top - bottom);
        xf.m[1][3] = -(top + bottom) / (top - bottom);

        xf.m[2][2] = -2.0f / (far - near);
        xf.m[2][3] = -(far + near) / (far - near);

        return xf;
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
