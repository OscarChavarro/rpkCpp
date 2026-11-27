import { Double } from "../../../../java/lang/Double.js";
import { Math } from "../../../../java/lang/Math.js";

/** `DBL_MIN` of C, the smallest positive normal binary64 value. */
const DBL_MIN = 2.2250738585072014e-308;

/**
 * Closed binary64 enclosure with one-ULP outward rounding per operation;
 * sums and products of point intervals that are proven exact stay points.
 * Invalid inputs, overflow and unsupported floating-point environments produce
 * an invalid interval. The implementation requires IEEE-754 binary64,
 * round-to-nearest, gradual underflow and correctly rounded elementary sqrt.
 */
export class FloatingInterval {
    private lower: number;
    private upper: number;
    private valid: boolean;

    public constructor();
    /** Private in C++: `FloatingInterval(double lowerValue, double upperValue, bool isValid)`. */
    public constructor(lowerValue: number, upperValue: number, isValid: boolean);
    public constructor(lowerValue = 0, upperValue = 0, isValid = false) {
        this.lower = lowerValue;
        this.upper = upperValue;
        this.valid = isValid;
    }

    /**
     * JavaScript numbers are always IEEE-754 binary64 with round-to-nearest
     * and correctly rounded `sqrt`; only gradual underflow is checked, as the
     * C++ port does after its compile-time checks.
     */
    public static isSupportedEnvironment(): boolean {
        const tiny = Double.MIN_VALUE;
        const one = 1;
        return tiny * one !== 0 && Math.nextAfter(0, 1) === Double.MIN_VALUE;
    }

    public static exact(value: number): FloatingInterval {
        return Double.isFinite(value) ? new FloatingInterval(value, value, true) : new FloatingInterval();
    }

    /** The closed interval [lowerValue, upperValue]; invalid unless finite and ordered. */
    public static spanning(lo: number, hi: number): FloatingInterval {
        return Double.isFinite(lo) && Double.isFinite(hi) && lo <= hi
            ? new FloatingInterval(lo, hi, true)
            : new FloatingInterval();
    }

    /**
     * An enclosure of [center - halfWidth, center + halfWidth]; exact when the
     * half width is zero, invalid for non-finite values or a negative width.
     */
    public static around(center: number, halfWidth: number): FloatingInterval {
        if (!Double.isFinite(center) || !Double.isFinite(halfWidth) || halfWidth < 0) return new FloatingInterval();
        if (halfWidth === 0) return FloatingInterval.exact(center);
        const low = FloatingInterval.exact(center).subtract(FloatingInterval.exact(halfWidth));
        const high = FloatingInterval.exact(center).add(FloatingInterval.exact(halfWidth));
        return low.valid && high.valid ? FloatingInterval.spanning(low.lower, high.upper) : new FloatingInterval();
    }

    private static rounded(lo: number, hi: number): FloatingInterval {
        if (!Double.isFinite(lo) || !Double.isFinite(hi) || lo > hi) return new FloatingInterval();
        lo = Math.nextAfter(lo, Double.NEGATIVE_INFINITY);
        hi = Math.nextAfter(hi, Double.POSITIVE_INFINITY);
        if (!Double.isFinite(lo) || !Double.isFinite(hi)) return new FloatingInterval();
        return new FloatingInterval(lo, hi, true);
    }

    public isValid(): boolean {
        return this.valid;
    }
    public getLower(): number {
        return this.lower;
    }
    public getUpper(): number {
        return this.upper;
    }

    public add(b: FloatingInterval): FloatingInterval {
        if (!this.valid || !b.valid) return new FloatingInterval();
        if (this.lower === this.upper && b.lower === b.upper) {
            // TwoSum: a zero rounding error means the point sum is exact.
            const sum = this.lower + b.lower;
            const shifted = sum - this.lower;
            const error = this.lower - (sum - shifted) + (b.lower - shifted);
            if (Double.isFinite(sum) && error === 0) return new FloatingInterval(sum, sum, true);
        }
        return FloatingInterval.rounded(this.lower + b.lower, this.upper + b.upper);
    }
    public subtract(b: FloatingInterval): FloatingInterval {
        if (!this.valid || !b.valid) return new FloatingInterval();
        return this.add(new FloatingInterval(-b.upper, -b.lower, true));
    }
    public multiply(b: FloatingInterval): FloatingInterval {
        if (!this.valid || !b.valid) return new FloatingInterval();
        if (this.lower === this.upper && b.lower === b.upper) {
            // A correctly rounded fused residual of zero means the product is exact.
            // Below 2^-969 the residual could underflow to zero, so do not trust it.
            const product = this.lower * b.lower;
            if (
                Double.isFinite(product) &&
                (product === 0
                    ? this.lower === 0 || b.lower === 0
                    : Math.abs(product) >= DBL_MIN * 9007199254740992.0) &&
                Math.fma(this.lower, b.lower, -product) === 0
            )
                return new FloatingInterval(product, product, true);
        }
        const values = [this.lower * b.lower, this.lower * b.upper, this.upper * b.lower, this.upper * b.upper];
        let lo = values[0]!,
            hi = values[0]!;
        for (let i = 1; i < 4; ++i) {
            if (values[i]! < lo) lo = values[i]!;
            if (values[i]! > hi) hi = values[i]!;
        }
        for (let i = 0; i < 4; ++i) if (!Double.isFinite(values[i]!)) return new FloatingInterval();
        return FloatingInterval.rounded(lo, hi);
    }
    public divide(b: FloatingInterval): FloatingInterval {
        if (!this.valid || !b.valid || (b.lower <= 0 && b.upper >= 0)) return new FloatingInterval();
        const values = [this.lower / b.lower, this.lower / b.upper, this.upper / b.lower, this.upper / b.upper];
        let lo = values[0]!,
            hi = values[0]!;
        for (let i = 1; i < 4; ++i) {
            if (values[i]! < lo) lo = values[i]!;
            if (values[i]! > hi) hi = values[i]!;
        }
        for (let i = 0; i < 4; ++i) if (!Double.isFinite(values[i]!)) return new FloatingInterval();
        return FloatingInterval.rounded(lo, hi);
    }
    public square(): FloatingInterval {
        if (!this.valid) return new FloatingInterval();
        const a = this.lower * this.lower,
            b = this.upper * this.upper;
        if (!Double.isFinite(a) || !Double.isFinite(b)) return new FloatingInterval();
        const hi = a > b ? a : b;
        const lo = this.lower <= 0 && this.upper >= 0 ? 0 : a < b ? a : b;
        const result = FloatingInterval.rounded(lo, hi);
        if (result.valid && result.lower < 0) result.lower = 0;
        return result;
    }
    public squareRoot(): FloatingInterval {
        // Intersect with the square-root domain. Callers must know that the
        // represented exact value is nonnegative (e.g. a sum of squares).
        if (!this.valid || this.upper < 0) return new FloatingInterval();
        const result = FloatingInterval.rounded(Math.sqrt(this.lower < 0 ? 0 : this.lower), Math.sqrt(this.upper));
        if (result.valid && result.lower < 0) result.lower = 0;
        return result;
    }
}
