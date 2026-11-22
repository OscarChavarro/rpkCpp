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

    /**
    Square of the length (the dot product with itself)
    */
    public float lengthSquared() {
        return x * x + y * y + z * z;
    }

    /**
    Sum of the absolute values of the components (the L1 norm)
    */
    public float manhattanLength() {
        return absolute(x) + absolute(y) + absolute(z);
    }

    /**
    Distance to the given point: the length of (other - this)
    */
    public float distance(Vector3Df other) {
        return other.subtract(this).length();
    }

    /**
    Square of the distance to the given point
    */
    public float distanceSquared(Vector3Df other) {
        return other.subtract(this).lengthSquared();
    }

    /**
    Scales by 1 / s, or returns this vector unchanged when s is within
    [-epsilon, epsilon]. The factor 1 / s is computed once and multiplies
    each component
    */
    public Vector3Df multiplyByInverse(float s, float epsilon) {
        final float factor = s < -epsilon || s > epsilon ? 1.0f / s : 1.0f;
        return new Vector3Df(factor * x, factor * y, factor * z);
    }

    /**
    Scales by the inverse of the length, or returns this vector unchanged
    when the length is within [-epsilon, epsilon]. Unlike normalized(), the
    threshold applies to the length, and a unit vector is also rescaled
    */
    public Vector3Df normalized(float epsilon) {
        return multiplyByInverse(length(), epsilon);
    }

    /**
    Index of the component of largest absolute value: 0 for x, 1 for y,
    2 for z; ties go to x before y and y before z
    */
    public int dominantAxis() {
        final float ax = absolute(x);
        final float ay = absolute(y);
        final float az = absolute(z);
        float m = ay > az ? ay : az;
        m = ax > m ? ax : m;
        if ( m == ax ) {
            return 0;
        }
        return m == ay ? 1 : 2;
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

    @Override
    public boolean equals(Object obj) {
        if ( this == obj ) {
            return true;
        }
        if ( !(obj instanceof Vector3Df other) ) {
            return false;
        }
        return Float.compare(x, other.x) == 0 &&
               Float.compare(y, other.y) == 0 &&
               Float.compare(z, other.z) == 0;
    }

    @Override
    public int hashCode() {
        int result = Float.hashCode(x);
        result = 31 * result + Float.hashCode(y);
        result = 31 * result + Float.hashCode(z);
        return result;
    }

    private static float absolute(float a) {
        return a < 0.0f ? -a : a;
    }
}
