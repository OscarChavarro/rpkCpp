package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import java.util.Objects;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.FundamentalEntity;

/**
Represents a quaternion in vector-scalar form.

<p>For rotation use cases the quaternion is expected to have unit length.
*/
public final class Quaterniond extends FundamentalEntity
{
    @Serial
    private static final long serialVersionUID = 20260419L;

    private final Vector3Dd direction;
    private final double magnitude;

    public Quaterniond()
    {
        this(new Vector3Dd(0, 0, 0), 0);
    }

    public Quaterniond(Vector3Dd direction, double magnitude)
    {
        this.direction = Vector3Dd.copyOf(Objects.requireNonNull(direction, "Quaterniond direction cannot be null"));
        this.magnitude = magnitude;
    }

    public Quaterniond(Quaterniond other)
    {
        this(Objects.requireNonNull(other, "Quaterniond to copy cannot be null").direction,
             other.magnitude);
    }

    public static Quaterniond copyOf(Quaterniond other)
    {
        return new Quaterniond(Objects.requireNonNull(other, "Quaterniond to copy cannot be null"));
    }

    /**
    @return the squared Euclidean norm of this quaternion
    */
    public double lengthSquared()
    {
        return magnitude * magnitude + direction.dotProduct(direction);
    }

    /**
    @return the Euclidean norm of this quaternion
    */
    public double length()
    {
        return Math.sqrt(lengthSquared());
    }

    /**
    @return a normalized quaternion, or this quaternion if its norm is near zero
    */
    public Quaterniond normalized()
    {
        double l = length();
        if ( Math.abs(l) < VSDK.EPSILON ) {
            return this;
        }
        return new Quaterniond(direction.multiply(1/l), magnitude * (1/l));
    }

    /**
    @return the quaternion conjugate
    */
    public Quaterniond conjugated()
    {
        return new Quaterniond(direction.multiply(-1), magnitude);
    }

    /**
    Rotates the given vector by this quaternion.

    <p>This method assumes the quaternion has unit length. Callers on hot
    paths should normalize once when caching the quaternion and reuse it.

    @param vector vector to rotate
    @return rotated vector
    */
    public Vector3Dd rotate(Vector3Dd vector)
    {
        Vector3Dd normalizedAxis = direction;
        Vector3Dd uv = normalizedAxis.crossProduct(
            Objects.requireNonNull(vector, "Vector to rotate cannot be null"));
        Vector3Dd uuv = normalizedAxis.crossProduct(uv);

        return vector.add(uv.multiply(2 * magnitude)).add(uuv.multiply(2));
    }

    public Quaterniond withDirection(Vector3Dd newDirection)
    {
        return new Quaterniond(newDirection, magnitude);
    }

    public Quaterniond withMagnitude(double newMagnitude)
    {
        return new Quaterniond(direction, newMagnitude);
    }

    public Vector3Dd direction()
    {
        return direction;
    }

    public double magnitude()
    {
        return magnitude;
    }

    public boolean epsilonEquals(Quaterniond other)
    {
        return epsilonEquals(other, VSDK.EPSILON);
    }

    public boolean epsilonEquals(Quaterniond other, double epsilon)
    {
        if ( other == null ) {
            return false;
        }
        if ( epsilon < 0.0 ) {
            throw new IllegalArgumentException("epsilon must be >= 0");
        }
        return direction.epsilonEquals(other.direction, epsilon) &&
               Math.abs(magnitude - other.magnitude) <= epsilon;
    }

    @Override
    public String toString()
    {
        String msg;
        msg = direction + " / " + VSDK.formatDouble(magnitude);
        return msg;
    }
}
