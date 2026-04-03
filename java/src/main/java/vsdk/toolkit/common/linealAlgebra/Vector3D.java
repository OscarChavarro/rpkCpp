package vsdk.toolkit.common.linealAlgebra;

public class Vector3D {
    public double x;
    public double y;
    public double z;

    public Vector3D() {
        this(0.0, 0.0, 0.0);
    }

    public Vector3D(double x, double y, double z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public static Vector3D unitX() {
        return new Vector3D(1.0, 0.0, 0.0);
    }

    public static Vector3D unitY() {
        return new Vector3D(0.0, 1.0, 0.0);
    }

    public static Vector3D unitZ() {
        return new Vector3D(0.0, 0.0, 1.0);
    }

    public double norm2() {
        return x * x + y * y + z * z;
    }

    public double norm() {
        return Math.sqrt(norm2());
    }

    public Vector3D normalized() {
        double n = norm();
        if (n == 0.0) {
            return new Vector3D();
        }
        return new Vector3D(x / n, y / n, z / n);
    }

    @Override
    public String toString() {
        return "Vector3D{" +
            "x=" + x +
            ", y=" + y +
            ", z=" + z +
            '}';
    }
}
