package vsdk.toolkit.common.linealAlgebra;

public class Vector2Dd {
    public double u;
    public double v;

    public Vector2Dd() {
        u = 0.0;
        v = 0.0;
    }

    public static void set(Vector2Dd vector, double a, double b) {
        vector.u = a;
        vector.v = b;
    }

    public static void subtract(Vector2Dd p, Vector2Dd q, Vector2Dd r) {
        r.u = p.u - q.u;
        r.v = p.v - q.v;
    }

    public static void add(Vector2Dd p, Vector2Dd q, Vector2Dd r) {
        r.u = p.u + q.u;
        r.v = p.v + q.v;
    }

    public static void negate(Vector2Dd p) {
        p.u = -p.u;
        p.v = -p.v;
    }

    public static double determinant(Vector2Dd a, Vector2Dd b) {
        return a.u * b.v - a.v * b.u;
    }
}
