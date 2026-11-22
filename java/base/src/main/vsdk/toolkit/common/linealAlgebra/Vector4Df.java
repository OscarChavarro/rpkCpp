package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;

public final class Vector4Df extends FundamentalEntity {
    @Serial
    private static final long serialVersionUID = 20260514L;

    private final float x;
    private final float y;
    private final float z;
    private final float w;

    public Vector4Df(float x, float y, float z, float w) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.w = w;
    }

    public Vector4Df(Vector4Df other) {
        this(other.x, other.y, other.z, other.w);
    }

    public Vector4Df(Vector4Dd other) {
        this((float) other.x(), (float) other.y(), (float) other.z(), (float) other.w());
    }

    public Vector4Df(Vector3Df other) {
        this(other.x(), other.y(), other.z(), 1f);
    }

    public Vector4Df multiply(float a) {
        return new Vector4Df(a * x, a * y, a * z, a * w);
    }

    public Vector4Df dividedByW() {
        if ( Math.abs(w) < (float) VSDK.EPSILON ) return this;
        return new Vector4Df(x / w, y / w, z / w, 1f);
    }

    public float length() {
        return (float) Math.sqrt(x * x + y * y + z * z + w * w);
    }

    public Vector4Df add(Vector4Df b) {
        return new Vector4Df(x + b.x, y + b.y, z + b.z, w + b.w);
    }

    public Vector4Df withX(float nx) { return new Vector4Df(nx, y, z, w); }
    public Vector4Df withY(float ny) { return new Vector4Df(x, ny, z, w); }
    public Vector4Df withZ(float nz) { return new Vector4Df(x, y, nz, w); }
    public Vector4Df withW(float nw) { return new Vector4Df(x, y, z, nw); }
    public float x() { return x; }
    public float y() { return y; }
    public float z() { return z; }
    public float w() { return w; }

    public boolean epsilonEquals(Vector4Df other) {
        return epsilonEquals(other, (float) VSDK.EPSILON);
    }

    public boolean epsilonEquals(Vector4Df other, float epsilon) {
        if ( other == null ) return false;
        if ( epsilon < 0f ) throw new IllegalArgumentException("epsilon must be >= 0");
        return Math.abs(x - other.x) <= epsilon &&
               Math.abs(y - other.y) <= epsilon &&
               Math.abs(z - other.z) <= epsilon &&
               Math.abs(w - other.w) <= epsilon;
    }
}
