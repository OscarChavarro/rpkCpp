package vsdk.toolkit.skin;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

/**
Tests whether two floating point numbers are equal within the given tolerance
*/

/**
Returns whether the first floating point value is greater than the second one.
*/

public final class Numeric {
    public static final double HUGE_DOUBLE_VALUE = 1e30;
    public static final double EPSILON = 1e-6;

    private Numeric() {
    }

    public static boolean doubleEqual(double a, double b, double tolerance) {
        return (a - b) > -tolerance && (a - b) < tolerance;
    }

    /**
    Two vectors are equal if their components are equal within the given
    tolerance, compared in double precision as doubleEqual does
    */
    public static boolean vectorsEqual(Vector3Dd a, Vector3Dd b, double tolerance) {
        return doubleEqual(a.x(), b.x(), tolerance) &&
            doubleEqual(a.y(), b.y(), tolerance) &&
            doubleEqual(a.z(), b.z(), tolerance);
    }

    public static boolean floatCompare(double x, double y) {
        return x > y;
    }

    public static void roundDeltaToZero(double[] x, double epsilon) {
        if (x == null || x.length == 0) {
            return;
        }
        if (x[0] <= epsilon && x[0] >= -epsilon) {
            x[0] = 0.0;
        }
    }
}
