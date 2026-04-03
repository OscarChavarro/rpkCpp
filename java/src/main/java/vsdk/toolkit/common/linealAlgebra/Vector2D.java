package vsdk.toolkit.common.linealAlgebra;

public class Vector2D {
    public float x;
    public float y;

    public Vector2D() {
        this(0.0f, 0.0f);
    }

    public Vector2D(float x, float y) {
        this.x = x;
        this.y = y;
    }

    public Vector2D(double x, double y) {
        this((float)x, (float)y);
    }

    public static void difference(Vector2D a, Vector2D b, Vector2D o) {
        o.x = a.x - b.x;
        o.y = a.y - b.y;
    }

    public static float norm2(Vector2D d) {
        return d.x * d.x + d.y * d.y;
    }

    @Override
    public String toString() {
        return "Vector2D{" +
            "x=" + x +
            ", y=" + y +
            '}';
    }
}
