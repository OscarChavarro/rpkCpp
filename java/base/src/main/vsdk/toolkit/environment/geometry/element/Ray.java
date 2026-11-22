package vsdk.toolkit.environment.geometry.element;
import java.io.Serial;
import java.util.Objects;

import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.RaytraceStatistics;

/**
 This class models a mathematical RAY.
 */
public final class Ray extends FundamentalEntity
{
    @Serial
    private static final long serialVersionUID = 20060502L;
    private static final double UNIT_DIRECTION_TOLERANCE = 1e-12;

    private final Vector3Dd origin;
    private final Vector3Dd direction;
    private final double t;

    public Ray()
    {
        this(new Vector3Dd(0, 0, 0), new Vector3Dd(1, 0, 0), 0.0);
    }

    public Ray(Vector3Dd origin, Vector3Dd direction)
    {
        this(origin, direction, 0.0);
    }

    public Ray(Vector3Dd origin, Vector3Dd direction, double t)
    {
        this(origin, direction, t, false);
    }

    public Ray(Ray b)
    {
        this(Objects.requireNonNull(b, "Ray to copy cannot be null").origin,
             b.direction,
             b.t,
             true);
    }

    /**
    Explicit affine construction: preserve direction magnitude and t.
    Legacy constructors retain their historical normalization behavior.
    */
    public static Ray fromAffine(Vector3Dd origin, Vector3Dd direction, double t)
    {
        return new Ray(origin, direction, t, true);
    }

    public static Ray fromAffine(Vector3Dd origin, Vector3Dd direction)
    {
        return fromAffine(origin, direction, 0.0);
    }

    public static Ray copyOf(Ray other)
    {
        return Objects.requireNonNull(other, "Ray to copy cannot be null");
    }

    public Ray withOrigin(Vector3Dd newOrigin)
    {
        return new Ray(newOrigin, direction, t, true);
    }

    // Copies replace only the named component; they never renormalize.
    public Ray withDirection(Vector3Dd newDirection)
    {
        return new Ray(origin, newDirection, t, true);
    }

    public Ray withT(double newT)
    {
        RaytraceStatistics.recordRayWithT();
        return new Ray(origin, direction, newT, true);
    }

    public Vector3Dd getOrigin()
    {
        return origin;
    }

    public Vector3Dd getDirection()
    {
        return direction;
    }

    public double getT()
    {
        return t;
    }

    private Ray(
        Vector3Dd origin,
        Vector3Dd direction,
        double t,
        boolean directionAlreadyNormalized)
    {
        this.origin = Objects.requireNonNull(origin, "Ray origin cannot be null");
        this.direction = directionAlreadyNormalized ?
            Objects.requireNonNull(direction, "Ray direction cannot be null") :
            normalizeDirection(direction);
        this.t = t;
    }

    private static Vector3Dd normalizeDirection(Vector3Dd direction)
    {
        Vector3Dd candidate =
            Objects.requireNonNull(direction, "Ray direction cannot be null");
        double lengthSquared = candidate.dotProduct(candidate);
        if ( lengthSquared <= VSDK.EPSILON ) {
            return candidate;
        }
        if ( Math.abs(lengthSquared - 1.0) <= UNIT_DIRECTION_TOLERANCE ) {
            return candidate;
        }
        return candidate.multiply(1.0 / Math.sqrt(lengthSquared));
    }

    @Override
    public boolean equals(Object obj)
    {
        if ( this == obj ) {
            return true;
        }
        if ( !(obj instanceof Ray other) ) {
            return false;
        }
        return Double.compare(t, other.t) == 0 &&
               origin.equals(other.origin) &&
               direction.equals(other.direction);
    }

    @Override
    public int hashCode()
    {
        return Objects.hash(origin, direction, t);
    }

    /**
    Provides an object to text report conversion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human-readable representation of current Ray
    */
    @Override
    public String toString()
    {
        return "Ray Origin: " + origin + "; Direction: " + direction + " T: " + VSDK.formatDouble(t);
    }
}
