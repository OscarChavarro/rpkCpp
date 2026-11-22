package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;

public final class Vector3Df extends FundamentalEntity {
    @Serial
    private static final long serialVersionUID = 20260514L;

    private final float x;
    private final float y;
    private final float z;

    public Vector3Df() {
        this(0f, 0f, 0f);
    }

    public Vector3Df(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public Vector3Df(Vector3Df other) {
        this(other.x, other.y, other.z);
    }

    public Vector3Df(Vector3Dd other) {
        this((float) other.x(), (float) other.y(), (float) other.z());
    }

    public Vector3Df multiply(float a) {
        return new Vector3Df(a * x, a * y, a * z);
    }

    public Vector3Df crossProduct(Vector3Df other) {
        return new Vector3Df(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    public float dotProduct(Vector3Df other) {
        return x * other.x + y * other.y + z * other.z;
    }

    public Vector3Df normalized() {
        float t = x * x + y * y + z * z;
        if ( Math.abs(t) < (float) VSDK.EPSILON ) return this;
        if ( t != 0f && t != 1f ) t = (float) (1.0 / Math.sqrt(t));
        return new Vector3Df(x * t, y * t, z * t);
    }

    public float length() {
        return (float) Math.sqrt(x * x + y * y + z * z);
    }

    public Vector3Df add(Vector3Df b) {
        return new Vector3Df(x + b.x, y + b.y, z + b.z);
    }

    public Vector3Df subtract(Vector3Df b) {
        return new Vector3Df(x - b.x, y - b.y, z - b.z);
    }

    public Vector3Df withX(float nx) { return new Vector3Df(nx, y, z); }
    public Vector3Df withY(float ny) { return new Vector3Df(x, ny, z); }
    public Vector3Df withZ(float nz) { return new Vector3Df(x, y, nz); }
    public float x() { return x; }
    public float y() { return y; }
    public float z() { return z; }

    public boolean epsilonEquals(Vector3Df other) {
        return epsilonEquals(other, (float) VSDK.EPSILON);
    }

    public boolean epsilonEquals(Vector3Df other, float epsilon) {
        if ( other == null ) return false;
        if ( epsilon < 0f ) throw new IllegalArgumentException("epsilon must be >= 0");
        return Math.abs(x - other.x) <= epsilon &&
               Math.abs(y - other.y) <= epsilon &&
               Math.abs(z - other.z) <= epsilon;
    }
}
