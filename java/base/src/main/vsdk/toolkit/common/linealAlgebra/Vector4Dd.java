package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import java.util.Objects;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.FundamentalEntity;

public final class Vector4Dd extends FundamentalEntity
{
    @Serial
    private static final long serialVersionUID = 20061103L;

    private final double x;
    private final double y;
    private final double z;
    private final double w;

    public Vector4Dd(double x, double y, double z, double w) {
        this.x = x; this.y = y; this.z = z;
        this.w = w;
    }

    public Vector4Dd(Vector4Dd other) {
        this(Objects.requireNonNull(other, "Vector4Dd to copy cannot be null").x,
             other.y,
             other.z,
             other.w);
    }

    public Vector4Dd(Vector3Dd other) {
        this.x = other.x();
        this.y = other.y();
        this.z = other.z();
        w = 1;
    }

    public Vector4Dd multiply(double a) {
        return new Vector4Dd(a * x, a * y, a * z, a * w);
    }

    public Vector4Dd dividedByW() {
        if ( Math.abs(w) < VSDK.EPSILON ) return this;
        return new Vector4Dd(x / w, y / w, z / w, 1);
    }

    public double length() {
        return Math.sqrt(x*x + y*y + z*z + w*w);
    }

    public Vector4Dd add(Vector4Dd b)
    {
        return new Vector4Dd(x + b.x, y + b.y, z + b.z, w + b.w);
    }

    public Vector4Dd withX(double nx) { return new Vector4Dd(nx, y, z, w); }
    public Vector4Dd withY(double ny) { return new Vector4Dd(x, ny, z, w); }
    public Vector4Dd withZ(double nz) { return new Vector4Dd(x, y, nz, w); }
    public Vector4Dd withW(double nw) { return new Vector4Dd(x, y, z, nw); }
    public double x() { return x; }
    public double y() { return y; }
    public double z() { return z; }
    public double w() { return w; }

    public boolean epsilonEquals(Vector4Dd other)
    {
        return epsilonEquals(other, VSDK.EPSILON);
    }

    public boolean epsilonEquals(Vector4Dd other, double epsilon)
    {
        if ( other == null ) {
            return false;
        }
        if ( epsilon < 0.0 ) {
            throw new IllegalArgumentException("epsilon must be >= 0");
        }
        return Math.abs(x - other.x) <= epsilon &&
               Math.abs(y - other.y) <= epsilon &&
               Math.abs(z - other.z) <= epsilon &&
               Math.abs(w - other.w) <= epsilon;
    }

    /**
    Provides an object to text report conversion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human-readable representation of current vector
    */
    @Override
    public String toString()
    {
        String msg;

        msg = "<" + VSDK.formatDouble(x) + ", " + VSDK.formatDouble(y) +
              ", " + VSDK.formatDouble(z) + ", " + VSDK.formatDouble(w) + ">";

        return msg;
    }

    @Override
    public boolean equals(Object obj) {
        if ( this == obj ) return true;
        if ( !(obj instanceof Vector4Dd other) ) return false;
        return Double.compare(x, other.x) == 0 &&
               Double.compare(y, other.y) == 0 &&
               Double.compare(z, other.z) == 0 &&
               Double.compare(w, other.w) == 0;
    }

    @Override
    public int hashCode() {
        int result = Double.hashCode(x);
        result = 31 * result + Double.hashCode(y);
        result = 31 * result + Double.hashCode(z);
        result = 31 * result + Double.hashCode(w);
        return result;
    }
}
