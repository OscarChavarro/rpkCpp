package vsdk.toolkit.environment.geometry.element;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

/**
This class models a mathematical RAY.
*/
public class Ray {
    private static final double UNIT_DIRECTION_TOLERANCE = 1e-12;

    private Vector3Dd origin;
    private Vector3Dd direction;
    private double t;

    public Ray() {
        this(new Vector3Dd(0, 0, 0), new Vector3Dd(1, 0, 0), 0.0);
    }

    public Ray(Vector3Dd origin, Vector3Dd direction) {
        this(origin, direction, 0.0);
    }

    public Ray(Vector3Dd origin, Vector3Dd direction, double t) {
        this.origin = origin;
        this.direction = normalizeDirection(direction);
        this.t = t;
    }

    /**
    Explicit affine construction: preserve direction magnitude and t.
    Legacy constructors retain their historical normalization behavior.
    */
    public static Ray fromAffine(Vector3Dd origin, Vector3Dd direction) {
        return fromAffine(origin, direction, 0.0);
    }

    public static Ray fromAffine(Vector3Dd origin, Vector3Dd direction, double t) {
        Ray result = new Ray();
        result.setOriginAndDirection(origin, direction);
        result.setT(t);
        return result;
    }

    public static Ray copyOf(Ray other) {
        Ray result = new Ray();
        result.origin = other.origin;
        result.direction = other.direction;
        result.t = other.t;
        return result;
    }

    /**
    Copies replace only the named component; they never renormalize.
    */
    public Ray withOrigin(Vector3Dd newOrigin) {
        Ray result = copyOf(this);
        result.origin = newOrigin;
        return result;
    }

    public Ray withDirection(Vector3Dd newDirection) {
        Ray result = copyOf(this);
        result.direction = newDirection;
        return result;
    }

    public Ray withT(double newT) {
        Ray result = copyOf(this);
        result.t = newT;
        return result;
    }

    public Vector3Dd getOrigin() {
        return origin;
    }

    public Vector3Dd getDirection() {
        return direction;
    }

    public double getT() {
        return t;
    }

    public void setOrigin(Vector3Dd newOrigin) {
        origin = newOrigin;
    }

    public void setDirection(Vector3Dd newDirection) {
        direction = newDirection;
    }

    public void setT(double newT) {
        t = newT;
    }

    public void setOriginAndDirection(Vector3Dd newOrigin, Vector3Dd newDirection) {
        origin = newOrigin;
        direction = newDirection;
    }

    private static Vector3Dd normalizeDirection(Vector3Dd direction) {
        double lengthSquared = direction.dotProduct(direction);
        if ( lengthSquared <= VSDK.EPSILON ) {
            return direction;
        }
        if ( Math.abs(lengthSquared - 1.0) <= UNIT_DIRECTION_TOLERANCE ) {
            return direction;
        }
        return direction.multiply(1.0 / Math.sqrt(lengthSquared));
    }

    @Override
    public boolean equals(Object obj) {
        if ( this == obj ) {
            return true;
        }
        if ( !(obj instanceof Ray other) ) {
            return false;
        }
        return t == other.t && origin.equals(other.origin) && direction.equals(other.direction);
    }

    @Override
    public int hashCode() {
        int result = Double.hashCode(t);
        result = 31 * result + origin.hashCode();
        result = 31 * result + direction.hashCode();
        return result;
    }

    /**
    Provides an object to text report conversion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human-readable representation of current Ray
    */
    @Override
    public String toString() {
        return "Ray Origin: <" + VSDK.formatDouble(origin.x()) + ", " +
            VSDK.formatDouble(origin.y()) + ", " +
            VSDK.formatDouble(origin.z()) + ">; Direction: <" +
            VSDK.formatDouble(direction.x()) + ", " +
            VSDK.formatDouble(direction.y()) + ", " +
            VSDK.formatDouble(direction.z()) + "> T: " +
            VSDK.formatDouble(t);
    }
}
