package vsdk.toolkit.common.linealAlgebra;

public class Vector2D {
    public double x;
    public double y;

    public Vector2D() {
        this(0.0, 0.0);
    }

    public Vector2D(double x, double y) {
        this.x = x;
        this.y = y;
    }

    @Override
    public String toString() {
        return "Vector2D{" +
            "x=" + x +
            ", y=" + y +
            '}';
    }
}
