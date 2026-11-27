package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import java.util.Objects;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.FundamentalEntity;

/**
Class Vector3Dd represents a one dimensional array of three values, usually
to be interpreted as:
  - A column vector of 1x3 positions, useful in linear algebra computations.
  - A point in the R3 Euclidean space
As current class is supposed to be used in the context of computer graphics,
array elements are not indexed, to say from 0 to 2, but are instead named
with the usual 3D axis labels `x`, `y` and `z`.
This is one of the most fundamental classes in VitralSDK toolkit, and its
attributes are usually accessed in the inner loops of computational intensive
calculations.

Lack of get/set method enforces a direct attribute access programming style
which will lend to shorter code.
*/
public final class Vector3Dd extends FundamentalEntity
{
    // Check the general attribute description in superclass Entity.
    @Serial
    private static final long serialVersionUID = 20260419L;

    private final double x;
    private final double y;
    private final double z;

    /**
    The default Vector3Dd value is the zero value
    */
    public Vector3Dd() {
        this(0, 0, 0);
    }

    /**
    @param x X coordinate
    @param y Y coordinate
    @param z Z coordinate
    */
    public Vector3Dd(double x, double y, double z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public Vector3Dd(Vector3Dd other) {
        this(Objects.requireNonNull(other, "Vector3Dd to copy cannot be null").x,
             other.y,
             other.z);
    }

    public static Vector3Dd copyOf(Vector3Dd other) {
        return new Vector3Dd(Objects.requireNonNull(other, "Vector3Dd to copy cannot be null"));
    }

    public Vector3Dd multiply(double a) {
        return new Vector3Dd(a * x, a * y, a * z);
    }

    /**
    Componentwise product, as cpp `Vector3Dd::multiply(const Vector3Dd&)`.
    @param other the second factor
    @return (x*other.x, y*other.y, z*other.z)
    */
    public Vector3Dd multiply(Vector3Dd other) {
        return new Vector3Dd(x * other.x, y * other.y, z * other.z);
    }

    /**
    As cpp `Vector3Dd::midpoint`.
    @param other the second point
    @return 0.5*(this + other), componentwise
    */
    public Vector3Dd midpoint(Vector3Dd other) {
        return new Vector3Dd(
            0.5 * (x + other.x), 0.5 * (y + other.y), 0.5 * (z + other.z));
    }

    /**
    As cpp `Vector3Dd::normalizedFast`: divides by the length with no
    degeneracy guard.
    @return this vector divided by its length
    */
    public Vector3Dd normalizedFast() {
        final double vTemp = Math.sqrt(x*x + y*y + z*z);
        return new Vector3Dd(x / vTemp, y / vTemp, z / vTemp);
    }

    /**
    @param other the second vector in cross product
    @return Vector3Dd, the result of the operation
    */
    public Vector3Dd crossProduct(Vector3Dd other) {
        return new Vector3Dd(y*other.z - z*other.y, z*other.x - x*other.z, x*other.y - y*other.x);
    }

    /**
    @param other Vector3Dd
    @return dot product between two vectors
    */
    public double dotProduct(Vector3Dd other) {
        return (x * other.x + y * other.y + z * other.z);
    }

    public Vector3Dd normalized() {
        double t = x*x + y*y + z*z;
        if ( t <= VSDK.EPSILON * VSDK.EPSILON ) return this;
        if (t != 0 && t != 1) t = (1.0 / Math.sqrt(t));
        return new Vector3Dd(x * t, y * t, z * t);
    }

    /**
    @return current vector length
    */
    public double length() {
        return Math.sqrt(x*x + y*y + z*z);
    }

    public static double distance(Vector3Dd a, Vector3Dd b) {
        return Math.sqrt((a.x - b.x)*(a.x - b.x) +
                         (a.y - b.y)*(a.y - b.y) +
                         (a.z - b.z)*(a.z - b.z));
    }

    public Vector3Dd add(Vector3Dd b)
    {
        return new Vector3Dd(x + b.x, y + b.y, z + b.z);
    }

    public Vector3Dd subtract(Vector3Dd b)
    {
        return new Vector3Dd(x - b.x, y - b.y, z - b.z);
    }

    /**
    Square of the length (the dot product with itself)
    */
    public double lengthSquared() {
        return x * x + y * y + z * z;
    }

    /**
    Sum of the absolute values of the components (the L1 norm)
    */
    public double manhattanLength() {
        return absolute(x) + absolute(y) + absolute(z);
    }

    /**
    Distance to the given point: the length of (other - this)
    */
    public double distance(Vector3Dd other) {
        return other.subtract(this).length();
    }

    /**
    Square of the distance to the given point
    */
    public double distanceSquared(Vector3Dd other) {
        return other.subtract(this).lengthSquared();
    }

    /**
    Scales by 1 / s, or returns this vector unchanged when s is within
    [-epsilon, epsilon]. The factor 1 / s is computed once and multiplies
    each component
    */
    public Vector3Dd multiplyByInverse(double s, double epsilon) {
        final double factor = s < -epsilon || s > epsilon ? 1.0 / s : 1.0;
        return new Vector3Dd(factor * x, factor * y, factor * z);
    }

    /**
    Scales by the inverse of the length, or returns this vector unchanged
    when the length is within [-epsilon, epsilon]. Unlike normalized(), the
    threshold applies to the length, and a unit vector is also rescaled
    */
    public Vector3Dd normalized(double epsilon) {
        return multiplyByInverse(length(), epsilon);
    }

    /**
    Index of the component of largest absolute value: 0 for x, 1 for y,
    2 for z; ties go to x before y and y before z
    */
    public int dominantAxis() {
        final double ax = absolute(x);
        final double ay = absolute(y);
        final double az = absolute(z);
        double m = ay > az ? ay : az;
        m = ax > m ? ax : m;
        if ( m == ax ) {
            return 0;
        }
        return m == ay ? 1 : 2;
    }

    /**
    The component of the given axis: 0 for x, 1 for y, 2 for z
    */
    public double component(int axis) {
        return axis == 0 ? x : (axis == 1 ? y : z);
    }

    public float[] exportToFloatArrayVector()
    {
        return new float[]{(float)x, (float)y, (float)z, 1};
    }

    /**
    Provides an object to text report a convert, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human-readable representation of current vector
    */
    @Override
    public String toString()
    {
        String msg;

        msg = "<" + VSDK.formatDouble(x) + ", " + VSDK.formatDouble(y) +
              ", " + VSDK.formatDouble(z) + ">";

        return msg;
    }

    /**
    Taking current vector as tail-anchored to the origin, this method
    calculates the theta angle (in radians) of the tip, corresponding
    to tip coordinate <x, y, z>, in spherical coordinates <r, theta, phi>.
    Note that theta goes from 0 to 2*PI, and correspond to an axis of
    rotation <0, 0, 1>.
    POST: 0 <= theta <= 2*PI
    @return an angle with theta angle spherical coordinate for the point at
    the tip of current vector, when vector tail is anchored at the origin
    */
    public double obtainSphericalThetaAngle()
    {
        double val;
        if ( Math.abs(x) > VSDK.EPSILON ) {
            if ( x > 0 ) val = Math.atan(y/x);
            else val = Math.PI + Math.atan(y/x);
        }
        else if ( y > 0 ) {
            val = Math.PI/2;
        }
        else {
            val = Math.PI + Math.PI/2;
        }
        while ( val < 0 ) val += 2*Math.PI;
        while ( val > 2*Math.PI ) val -= 2*Math.PI;
        return val;
    }

    /**
    Taking current vector as tail-anchored to the origin, this method
    calculates the phi angle (in radians) of the tip, corresponding
    to tip coordinate <x, y, z>, in spherical coordinates <r, theta, phi>.
    Note phi goes from 0 to PI.
    @return an angle with phi angle spherical coordinate for the point at
    the tip of current vector, when vector tail is anchored at the origin
    */
    public double obtainSphericalPhiAngle()
    {
        double r = length();

        if ( r < VSDK.EPSILON ) return 0;

        return Math.acos(z/r);
    }

    public static Vector3Dd fromSpherical(double r, double theta, double phi)
    {
        return new Vector3Dd(
            r * Math.sin(phi) * Math.cos(theta),
            r * Math.sin(phi) * Math.sin(theta),
            r * Math.cos(phi)
        );
    }

    public Vector3Dd withX(double nx) {
        return new Vector3Dd(nx, y, z);
    }

    public Vector3Dd withY(double ny) {
        return new Vector3Dd(x, ny, z);
    }

    public Vector3Dd withZ(double nz) {
        return new Vector3Dd(x, y, nz);
    }

    public double x() {
        return x;
    }

    public double y() {
        return y;
    }

    public double z() {
        return z;
    }

    public boolean epsilonEquals(Vector3Dd other)
    {
        return epsilonEquals(other, VSDK.EPSILON);
    }

    public boolean epsilonEquals(Vector3Dd other, double epsilon)
    {
        if ( other == null ) {
            return false;
        }
        if ( epsilon < 0.0 ) {
            throw new IllegalArgumentException("epsilon must be >= 0");
        }
        return Math.abs(x - other.x) <= epsilon &&
               Math.abs(y - other.y) <= epsilon &&
               Math.abs(z - other.z) <= epsilon;
    }

    @Override
    public boolean equals(Object obj) {
        if ( this == obj ) {
            return true;
        }
        if ( !(obj instanceof Vector3Dd other) ) {
            return false;
        }
        return Double.compare(x, other.x) == 0 &&
               Double.compare(y, other.y) == 0 &&
               Double.compare(z, other.z) == 0;
    }

    @Override
    public int hashCode() {
        int result = Double.hashCode(x);
        result = 31 * result + Double.hashCode(y);
        result = 31 * result + Double.hashCode(z);
        return result;
    }

    private static double absolute(double a) {
        return a < 0.0 ? -a : a;
    }
}
