package vsdk.toolkit.common.linealAlgebra;

/**
Tests whether two floating point numbers are equal within the given tolerance
*/

/**
Returns whether the first floating point value is greater than the second one.
*/

public final class Numeric {
    public static final double HUGE_DOUBLE_VALUE = 1e30;
    public static final float HUGE_FLOAT_VALUE = Float.MAX_VALUE;
    public static final double EPSILON = 1e-6;
    public static final float EPSILON_FLOAT = 1e-6f;

    private Numeric() {
    }

    public static boolean doubleEqual(double a, double b, double tolerance) {
        return (a - b) > -tolerance && (a - b) < tolerance;
    }

    public static boolean floatCompare(float x, float y) {
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
