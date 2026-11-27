package vsdk.toolkit.common.numerics;

/**
Closed binary64 enclosure with one-ULP outward rounding per operation;
sums and products of point intervals that are proven exact stay points.
Invalid inputs, overflow and unsupported floating-point environments produce
an invalid interval. The implementation requires IEEE-754 binary64,
round-to-nearest, gradual underflow and correctly rounded elementary sqrt.
*/
public final class FloatingInterval {
    private static final double DBL_MIN = Double.MIN_NORMAL;

    private double lower;
    private double upper;
    private boolean valid;

    private FloatingInterval(double lo, double hi, boolean ok) { lower = lo; upper = hi; valid = ok; }

    public FloatingInterval() { this(0, 0, false); }

    public static boolean isSupportedEnvironment()
    {
        // Java 17 floating-point arithmetic is always strict IEEE-754 binary64
        // with round-to-nearest; only the gradual underflow probes of cpp remain.
        double tiny = Double.MIN_VALUE;
        double one = 1;
        return tiny * one != 0 && Math.nextAfter(0.0, 1.0) == Double.MIN_VALUE;
    }

    public static FloatingInterval exact(double value)
    {
        return Double.isFinite(value) ? new FloatingInterval(value, value, true) : new FloatingInterval();
    }

    /** The closed interval [lowerValue, upperValue]; invalid unless finite and ordered. */
    public static FloatingInterval spanning(double lo, double hi)
    {
        return Double.isFinite(lo) && Double.isFinite(hi) && lo <= hi ?
            new FloatingInterval(lo, hi, true) : new FloatingInterval();
    }

    /**
    An enclosure of [center - halfWidth, center + halfWidth]; exact when the
    half width is zero, invalid for non-finite values or a negative width.
    */
    public static FloatingInterval around(double center, double halfWidth)
    {
        if (!Double.isFinite(center) || !Double.isFinite(halfWidth) || halfWidth < 0)
            return new FloatingInterval();
        if (halfWidth == 0) return exact(center);
        final FloatingInterval low = exact(center).subtract(exact(halfWidth));
        final FloatingInterval high = exact(center).add(exact(halfWidth));
        return low.valid && high.valid ? spanning(low.lower, high.upper) : new FloatingInterval();
    }

    private static FloatingInterval rounded(double lo, double hi)
    {
        if (!Double.isFinite(lo) || !Double.isFinite(hi) || lo > hi) return new FloatingInterval();
        lo = Math.nextAfter(lo, Double.NEGATIVE_INFINITY);
        hi = Math.nextAfter(hi, Double.POSITIVE_INFINITY);
        if (!Double.isFinite(lo) || !Double.isFinite(hi)) return new FloatingInterval();
        return new FloatingInterval(lo, hi, true);
    }

    public boolean isValid() { return valid; }
    public double getLower() { return lower; }
    public double getUpper() { return upper; }

    public FloatingInterval add(FloatingInterval b)
    {
        if (!valid || !b.valid) return new FloatingInterval();
        if (lower == upper && b.lower == b.upper) {
            // TwoSum: a zero rounding error means the point sum is exact.
            final double sum = lower + b.lower;
            final double shifted = sum - lower;
            final double error = (lower - (sum - shifted)) + (b.lower - shifted);
            if (Double.isFinite(sum) && error == 0) return new FloatingInterval(sum, sum, true);
        }
        return rounded(lower + b.lower, upper + b.upper);
    }

    public FloatingInterval subtract(FloatingInterval b)
    {
        if (!valid || !b.valid) return new FloatingInterval();
        return add(new FloatingInterval(-b.upper, -b.lower, true));
    }

    public FloatingInterval multiply(FloatingInterval b)
    {
        if (!valid || !b.valid) return new FloatingInterval();
        if (lower == upper && b.lower == b.upper) {
            // A correctly rounded fused residual of zero means the product is exact.
            // Below 2^-969 the residual could underflow to zero, so do not trust it.
            // cpp uses std::fma for this residual; Math.fma is its exact counterpart.
            final double product = lower * b.lower;
            if (Double.isFinite(product) && (product == 0 ? (lower == 0 || b.lower == 0) :
                Math.abs(product) >= DBL_MIN * 9007199254740992.0) && Math.fma(lower, b.lower, -product) == 0)
                return new FloatingInterval(product, product, true);
        }
        final double[] values = {lower * b.lower, lower * b.upper, upper * b.lower, upper * b.upper};
        double lo = values[0], hi = values[0];
        for (int i = 1; i < 4; ++i) { if (values[i] < lo) lo = values[i]; if (values[i] > hi) hi = values[i]; }
        for (int i = 0; i < 4; ++i) if (!Double.isFinite(values[i])) return new FloatingInterval();
        return rounded(lo, hi);
    }

    public FloatingInterval divide(FloatingInterval b)
    {
        if (!valid || !b.valid || (b.lower <= 0 && b.upper >= 0)) return new FloatingInterval();
        final double[] values = {lower / b.lower, lower / b.upper, upper / b.lower, upper / b.upper};
        double lo = values[0], hi = values[0];
        for (int i = 1; i < 4; ++i) { if (values[i] < lo) lo = values[i]; if (values[i] > hi) hi = values[i]; }
        for (int i = 0; i < 4; ++i) if (!Double.isFinite(values[i])) return new FloatingInterval();
        return rounded(lo, hi);
    }

    public FloatingInterval square()
    {
        if (!valid) return new FloatingInterval();
        final double a = lower * lower, b = upper * upper;
        if (!Double.isFinite(a) || !Double.isFinite(b)) return new FloatingInterval();
        final double hi = a > b ? a : b;
        final double lo = (lower <= 0 && upper >= 0) ? 0 : (a < b ? a : b);
        FloatingInterval result = rounded(lo, hi);
        if (result.valid && result.lower < 0) result.lower = 0;
        return result;
    }

    public FloatingInterval squareRoot()
    {
        // Intersect with the square-root domain. Callers must know that the
        // represented exact value is nonnegative (e.g. a sum of squares).
        if (!valid || upper < 0) return new FloatingInterval();
        FloatingInterval result = rounded(Math.sqrt(lower < 0 ? 0 : lower), Math.sqrt(upper));
        if (result.valid && result.lower < 0) result.lower = 0;
        return result;
    }
}
