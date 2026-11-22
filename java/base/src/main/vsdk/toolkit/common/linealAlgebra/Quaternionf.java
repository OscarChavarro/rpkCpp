package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import java.util.Objects;

import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;

public final class Quaternionf extends FundamentalEntity
{
    @Serial
    private static final long serialVersionUID = 20260514L;

    private final Vector3Df direction;
    private final float magnitude;

    public Quaternionf()
    {
        this(new Vector3Df(0f, 0f, 0f), 0f);
    }

    public Quaternionf(Vector3Df direction, float magnitude)
    {
        this.direction = new Vector3Df(Objects.requireNonNull(direction, "Quaternion direction cannot be null"));
        this.magnitude = magnitude;
    }

    public Quaternionf(Quaternionf other)
    {
        this(Objects.requireNonNull(other, "Quaternion to copy cannot be null").direction,
             other.magnitude);
    }

    public Quaternionf(Quaterniond other)
    {
        this(new Vector3Df(other.direction()), (float) other.magnitude());
    }

    public static Quaternionf copyOf(Quaternionf other)
    {
        return new Quaternionf(Objects.requireNonNull(other, "Quaternion to copy cannot be null"));
    }

    public float lengthSquared()
    {
        return magnitude * magnitude + direction.dotProduct(direction);
    }

    public float length()
    {
        return (float) Math.sqrt(lengthSquared());
    }

    public Quaternionf normalized()
    {
        float l = length();
        if ( Math.abs(l) < (float) VSDK.EPSILON ) {
            return this;
        }
        float inv = 1f / l;
        return new Quaternionf(direction.multiply(inv), magnitude * inv);
    }

    public Quaternionf conjugated()
    {
        return new Quaternionf(direction.multiply(-1f), magnitude);
    }

    public Vector3Df rotate(Vector3Df vector)
    {
        Vector3Df normalizedAxis = direction;
        Vector3Df uv = normalizedAxis.crossProduct(
            Objects.requireNonNull(vector, "Vector to rotate cannot be null"));
        Vector3Df uuv = normalizedAxis.crossProduct(uv);

        return vector.add(uv.multiply(2f * magnitude)).add(uuv.multiply(2f));
    }

    public Quaternionf withDirection(Vector3Df newDirection)
    {
        return new Quaternionf(newDirection, magnitude);
    }

    public Quaternionf withMagnitude(float newMagnitude)
    {
        return new Quaternionf(direction, newMagnitude);
    }

    public Vector3Df direction()
    {
        return direction;
    }

    public float magnitude()
    {
        return magnitude;
    }

    public boolean epsilonEquals(Quaternionf other)
    {
        return epsilonEquals(other, (float) VSDK.EPSILON);
    }

    public boolean epsilonEquals(Quaternionf other, float epsilon)
    {
        if ( other == null ) {
            return false;
        }
        if ( epsilon < 0.0f ) {
            throw new IllegalArgumentException("epsilon must be >= 0");
        }
        return direction.epsilonEquals(other.direction, epsilon) &&
               Math.abs(magnitude - other.magnitude) <= epsilon;
    }

    @Override
    public String toString()
    {
        return direction + " / " + VSDK.formatDouble(magnitude);
    }
}
