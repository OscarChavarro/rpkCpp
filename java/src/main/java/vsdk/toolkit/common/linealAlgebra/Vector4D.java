package vsdk.toolkit.common.linealAlgebra;

public class Vector4D {
    public float x;
    public float y;
    public float z;
    public float w;

    public Vector4D() {
        this(0.0f, 0.0f, 0.0f, 0.0f);
    }

    public Vector4D(float x, float y, float z, float w) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.w = w;
    }
}
