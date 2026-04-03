package vsdk.toolkit.common.linealAlgebra;

public class Matrix2x2 {
    public float[][] m = new float[2][2];
    public float[] t = new float[2];

    public void transformPoint2D(Vector2D src, Vector2D dst) {
        float outX = m[0][0] * src.x + m[0][1] * src.y + t[0];
        float outY = m[1][0] * src.x + m[1][1] * src.y + t[1];
        dst.x = outX;
        dst.y = outY;
    }

    public void matrix2DPreConcatTransform(Matrix2x2 xf1, Matrix2x2 xf) {
        Matrix2x2 tmpXf = new Matrix2x2();
        tmpXf.m[0][0] = m[0][0] * xf1.m[0][0] + m[0][1] * xf1.m[1][0];
        tmpXf.m[0][1] = m[0][0] * xf1.m[0][1] + m[0][1] * xf1.m[1][1];
        tmpXf.m[1][0] = m[1][0] * xf1.m[0][0] + m[1][1] * xf1.m[1][0];
        tmpXf.m[1][1] = m[1][0] * xf1.m[0][1] + m[1][1] * xf1.m[1][1];
        tmpXf.t[0] = m[0][0] * xf1.t[0] + m[0][1] * xf1.t[1] + t[0];
        tmpXf.t[1] = m[1][0] * xf1.t[0] + m[1][1] * xf1.t[1] + t[1];

        xf.m[0][0] = tmpXf.m[0][0];
        xf.m[0][1] = tmpXf.m[0][1];
        xf.m[1][0] = tmpXf.m[1][0];
        xf.m[1][1] = tmpXf.m[1][1];
        xf.t[0] = tmpXf.t[0];
        xf.t[1] = tmpXf.t[1];
    }
}
