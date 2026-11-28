// Shared binary64 bit views (single-threaded per realm).
const conversionBuffer = new ArrayBuffer(8);
const conversionFloat = new Float64Array(conversionBuffer);
const conversionBits = new BigUint64Array(conversionBuffer);
const conversionWords = new Uint32Array(conversionBuffer);
conversionFloat[0] = 1.0;
const highWord = conversionWords[1] === 0x3ff00000 ? 1 : 0;
const lowWord = 1 - highWord;

/** `FdLibm.__HI(double)`: the high-order 32 bits, as a signed Java int. */
const fdlibmHigh = (x: number): number => {
    conversionFloat[0] = x;
    return conversionWords[highWord]! | 0;
};

/** `FdLibm.__HI(double, int)`: `x` with its high-order 32 bits replaced. */
const fdlibmWithHigh = (x: number, high: number): number => {
    conversionFloat[0] = x;
    conversionWords[highWord] = high >>> 0;
    return conversionFloat[0]!;
};

/** A double from its two 32-bit words. */
const fromWords = (high: number, low: number): number => {
    conversionWords[highWord] = high >>> 0;
    conversionWords[lowWord] = low >>> 0;
    return conversionFloat[0]!;
};

/** `Math.powerOfTwoD(int)`: 2^n for a normal exponent, built from its bits. */
const powerOfTwoD = (n: number): number => fromWords((n + 1023) << 20, 0);

/** `FdLibm.__LO(double)`: the low-order 32 bits, as a signed Java int. */
const fdlibmLow = (x: number): number => {
    conversionFloat[0] = x;
    return conversionWords[lowWord]! | 0;
};

/** Java's `(int)` cast of a double: truncation, saturating, NaN to 0. */
const javaDoubleToInt = (d: number): number => {
    if (d !== d) return 0;
    if (d >= 2147483647) return 2147483647;
    if (d <= -2147483648) return -2147483648;
    return globalThis.Math.trunc(d) | 0;
};

// `FdLibm.Exp` constants, as exact decimal literals of the Java hex literals.
const EXP_HALF = [0.5, -0.5];
const EXP_HUGE = 1.0e300;
const EXP_TWOM1000 = 9.332636185032189e-302; // 0x1.0p-1000
const EXP_O_THRESHOLD = 709.782712893384; // 0x1.62e42fefa39efp9
const EXP_U_THRESHOLD = -745.1332191019411; // -0x1.74910d52d3051p9
const EXP_LN2HI = [0.6931471803691238, -0.6931471803691238]; // 0x1.62e42feep-1
const EXP_LN2LO = [1.9082149292705877e-10, -1.9082149292705877e-10]; // 0x1.a39ef35793c76p-33
const EXP_INVLN2 = 1.4426950408889634; // 0x1.71547652b82fep0
const EXP_P1 = 0.16666666666666602; // 0x1.555555555553ep-3
const EXP_P2 = -0.0027777777777015593; // -0x1.6c16c16bebd93p-9
const EXP_P3 = 6.613756321437934e-5; // 0x1.1566aaf25de2cp-14
const EXP_P4 = -1.6533902205465252e-6; // -0x1.bbd41c5d26bf1p-20
const EXP_P5 = 4.1381367970572385e-8; // 0x1.6376972bea4d0p-25
// fdlibm `e_sinh.c`
const SINH_SHUGE = 1.0e307;

// The hexadecimal floating-point literals of `FdLibm.Hypot`.
const HYPOT_TWO_MINUS_600 = powerOfTwoD(-600); // 0x1.0p-600
const HYPOT_TWO_PLUS_600 = powerOfTwoD(600); // 0x1.0p+600
const HYPOT_TWO_MINUS_500 = powerOfTwoD(-500); // 0x1.0p-500
const HYPOT_TWO_PLUS_1022 = powerOfTwoD(1022); // 0x1.0p1022
const HYPOT_ABOUT_TWO_PLUS_500 = fromWords(0x5f300000, 0xffffffff); // 0x1.00000_ffff_ffffp500

export class Math {
    public static readonly E = 2.7182818284590452354;
    public static readonly PI = 3.14159265358979323846;

    public static floor(a: number): number {
        return globalThis.Math.floor(a);
    }

    public static ceil(a: number): number {
        return globalThis.Math.ceil(a);
    }

    /**
    `Math.rint(double)`: the closest integer value, with ties rounding to the
    even one (so `rint(2.5)` is `2` and `rint(-2.5)` is `-2`), and NaN,
    infinities and signed zeros returned as given. A result of zero keeps the
    sign of the argument, as in the JDK (`rint(-0.4)` is `-0.0`).
    */
    public static rint(a: number): number {
        if (!globalThis.Number.isFinite(a) || a === 0) {
            return a;
        }
        const f = globalThis.Math.floor(a);
        const fraction = a - f;
        let result: number;
        if (fraction < 0.5) {
            result = f;
        } else if (fraction > 0.5) {
            result = f + 1;
        } else {
            result = f % 2 === 0 ? f : f + 1;
        }
        if (result === 0) {
            return a < 0 ? -0 : 0;
        }
        return result;
    }

    /**
    `Math.round(double)`: the closest integer, with ties rounding towards
    positive infinity (so `round(-2.5)` is `-2`, not `-3`), `0` for NaN, and
    saturation at the `long` range. JavaScript's `Math.round` already uses the
    same tie rule and handles `0.49999999999999994` the way the JDK does.
    */
    public static round(a: number): number {
        if (globalThis.Number.isNaN(a)) {
            return 0;
        }
        if (a <= -9223372036854775808) {
            return -9223372036854775808;
        }
        if (a >= 9223372036854775807) {
            return 9223372036854775807;
        }
        return globalThis.Math.round(a) + 0;
    }

    public static log(a: number): number {
        return globalThis.Math.log(a);
    }

    public static log10(a: number): number {
        return globalThis.Math.log10(a);
    }

    public static sin(a: number): number {
        return globalThis.Math.sin(a);
    }

    public static cos(a: number): number {
        return globalThis.Math.cos(a);
    }

    public static tan(a: number): number {
        return globalThis.Math.tan(a);
    }

    public static acos(a: number): number {
        return globalThis.Math.acos(a);
    }

    public static atan(a: number): number {
        return globalThis.Math.atan(a);
    }

    public static exp(a: number): number {
        return globalThis.Math.exp(a);
    }

    public static pow(a: number, e: number): number {
        return globalThis.Math.pow(a, e);
    }

    public static abs(a: number): number {
        return globalThis.Math.abs(a);
    }

    /**
    `Math.min(double, double)`: NaN if either argument is NaN, and -0.0 is
    smaller than 0.0. JavaScript's two-argument `Math.min` has exactly these
    semantics (and, for int values, those of `Math.min(int, int)`).
    */
    public static min(a: number, b: number): number {
        return globalThis.Math.min(a, b);
    }

    /** `Math.max(double, double)`: see `min`. */
    public static max(a: number, b: number): number {
        return globalThis.Math.max(a, b);
    }

    /** `Math.toRadians(double)`: `angdeg / 180.0 * PI`. */
    public static toRadians(angdeg: number): number {
        return (angdeg / 180.0) * Math.PI;
    }

    /** `Math.toDegrees(double)`: `angrad * 180.0 / PI`. */
    public static toDegrees(angrad: number): number {
        return (angrad * 180.0) / Math.PI;
    }

    /** `Math.floorDiv(int, int)`. */
    public static floorDiv(x: number, y: number): number {
        return globalThis.Math.floor(x / y);
    }

    /** `Math.floorMod(int, int)`: `x - floorDiv(x, y) * y`. */
    public static floorMod(x: number, y: number): number {
        return x - Math.floorDiv(x, y) * y;
    }

    /**
    `StrictMath.exp(double)`: JDK 17's `FdLibm.Exp.compute`, ported
    literally. It is not `Math.exp`, which HotSpot computes with a CPU
    intrinsic; Java uses it inside `cosh` and `sinh`. V8's `Math.exp` differs
    from it at `exp(1)`.
    */
    public static strictExp(x: number): number {
        let y: number;
        let hi = 0.0;
        let lo = 0.0;
        let c: number;
        let t: number;
        let k = 0;
        let xsb: number;
        let hx: number;

        hx = fdlibmHigh(x); /* high word of x */
        xsb = (hx >> 31) & 1; /* sign bit of x */
        hx &= 0x7fffffff; /* high word of |x| */

        /* filter out non-finite argument */
        if (hx >= 0x40862e42) {
            /* if |x| >= 709.78... */
            if (hx >= 0x7ff00000) {
                if (((hx & 0xfffff) | fdlibmLow(x)) !== 0) return x + x; /* NaN */
                else return xsb === 0 ? x : 0.0; /* exp(+-inf) = {inf, 0} */
            }
            if (x > EXP_O_THRESHOLD) return EXP_HUGE * EXP_HUGE; /* overflow */
            if (x < EXP_U_THRESHOLD) return EXP_TWOM1000 * EXP_TWOM1000; /* underflow */
        }

        /* argument reduction */
        if (hx > 0x3fd62e42) {
            /* if  |x| > 0.5 ln2 */
            if (hx < 0x3ff0a2b2) {
                /* and |x| < 1.5 ln2 */
                hi = x - EXP_LN2HI[xsb]!;
                lo = EXP_LN2LO[xsb]!;
                k = 1 - xsb - xsb;
            } else {
                k = javaDoubleToInt(EXP_INVLN2 * x + EXP_HALF[xsb]!);
                t = k;
                hi = x - t * EXP_LN2HI[0]!; /* t*ln2HI is exact here */
                lo = t * EXP_LN2LO[0]!;
            }
            x = hi - lo;
        } else if (hx < 0x3e300000) {
            /* when |x|<2**-28 */
            if (EXP_HUGE + x > 1.0) return 1.0 + x; /* trigger inexact */
        } else {
            k = 0;
        }

        /* x is now in primary range */
        t = x * x;
        c = x - t * (EXP_P1 + t * (EXP_P2 + t * (EXP_P3 + t * (EXP_P4 + t * EXP_P5))));
        if (k === 0) return 1.0 - ((x * c) / (c - 2.0) - x);
        else y = 1.0 - (lo - (x * c) / (2.0 - c) - hi);

        if (k >= -1021) {
            y = fdlibmWithHigh(y, (fdlibmHigh(y) + (k << 20)) | 0); /* add k to y's exponent */
            return y;
        } else {
            y = fdlibmWithHigh(y, (fdlibmHigh(y) + ((k + 1000) << 20)) | 0); /* add k to y's exponent */
            return y * EXP_TWOM1000;
        }
    }

    /**
    `Math.cosh(double)`: JDK 17 delegates it to `StrictMath.cosh`, the native
    fdlibm `__ieee754_cosh`, which this ports. It calls `strictExp` (fdlibm's
    `exp`) and `expm1`; V8's `Math.expm1` is fdlibm's and matches Java's bit
    for bit.
    */
    public static cosh(x: number): number {
        let t: number, w: number;
        let ix: number;

        ix = fdlibmHigh(x);
        ix &= 0x7fffffff;

        /* x is INF or NaN */
        if (ix >= 0x7ff00000) return x * x;

        /* |x| in [0,0.5*ln2], return 1+expm1(|x|)^2/(2*exp(|x|)) */
        if (ix < 0x3fd62e43) {
            t = globalThis.Math.expm1(globalThis.Math.abs(x));
            w = 1.0 + t;
            if (ix < 0x3c800000) return w; /* cosh(tiny) = 1 */
            return 1.0 + (t * t) / (w + w);
        }

        /* |x| in [0.5*ln2,22], return (exp(|x|)+1/exp(|x|)/2; */
        if (ix < 0x40360000) {
            t = Math.strictExp(globalThis.Math.abs(x));
            return 0.5 * t + 0.5 / t;
        }

        /* |x| in [22, log(maxdouble)] return half*exp(|x|) */
        if (ix < 0x40862e42) return 0.5 * Math.strictExp(globalThis.Math.abs(x));

        /* |x| in [log(maxdouble), overflowthresold] */
        if (ix < 0x408633ce || (ix === 0x408633ce && fdlibmLow(x) >>> 0 <= 0x8fb9f87d)) {
            w = Math.strictExp(0.5 * globalThis.Math.abs(x));
            t = 0.5 * w;
            return t * w;
        }

        /* |x| > overflowthresold, cosh(x) overflow */
        return EXP_HUGE * EXP_HUGE;
    }

    /**
    `Math.sinh(double)`: JDK 17 delegates it to `StrictMath.sinh`, the native
    fdlibm `__ieee754_sinh`, which this ports (see `cosh`).
    */
    public static sinh(x: number): number {
        let t: number, w: number, h: number;
        let ix: number, jx: number;

        jx = fdlibmHigh(x);
        ix = jx & 0x7fffffff;

        /* x is INF or NaN */
        if (ix >= 0x7ff00000) return x + x;

        h = 0.5;
        if (jx < 0) h = -h;
        /* |x| in [0,22], return sign(x)*0.5*(E+E/(E+1))) */
        if (ix < 0x40360000) {
            /* |x|<22 */
            if (ix < 0x3e300000)
                if (SINH_SHUGE + x > 1.0)
                    /* |x|<2**-28 */
                    return x; /* sinh(tiny) = tiny with inexact */
            t = globalThis.Math.expm1(globalThis.Math.abs(x));
            if (ix < 0x3ff00000) return h * (2.0 * t - (t * t) / (t + 1.0));
            return h * (t + t / (t + 1.0));
        }

        /* |x| in [22, log(maxdouble)] return 0.5*exp(|x|) */
        if (ix < 0x40862e42) return h * Math.strictExp(globalThis.Math.abs(x));

        /* |x| in [log(maxdouble), overflowthresold] */
        if (ix < 0x408633ce || (ix === 0x408633ce && fdlibmLow(x) >>> 0 <= 0x8fb9f87d)) {
            w = Math.strictExp(0.5 * globalThis.Math.abs(x));
            t = h * w;
            return t * w;
        }

        /* |x| > overflowthresold, sinh(x) overflow */
        return x * SINH_SHUGE;
    }

    /**
    `Math.hypot(double, double)`: JDK 17 delegates it to `StrictMath.hypot`,
    which is `FdLibm.Hypot.compute`. This is a literal port of that method;
    V8's `Math.hypot` rounds differently.
    */
    public static hypot(x: number, y: number): number {
        let a: number = globalThis.Math.abs(x);
        let b: number = globalThis.Math.abs(y);

        if (!globalThis.Number.isFinite(a) || !globalThis.Number.isFinite(b)) {
            if (a === Infinity || b === Infinity) return Infinity;
            else return a + b; // Propagate NaN significand bits
        }

        if (b > a) {
            const tmp = a;
            a = b;
            b = tmp;
        }

        // The high-order 32 bits of a and b, as Java ints.
        let ha: number = fdlibmHigh(a);
        let hb: number = fdlibmHigh(b);

        if (ha - hb > 0x3c00000) {
            return a + b; // x / y > 2**60
        }

        let k = 0;
        if (a > HYPOT_ABOUT_TWO_PLUS_500) {
            // a > ~2**500: scale a and b by 2**-600
            ha = (ha - 0x25800000) | 0;
            hb = (hb - 0x25800000) | 0;
            a = a * HYPOT_TWO_MINUS_600;
            b = b * HYPOT_TWO_MINUS_600;
            k += 600;
        }
        let t1: number, t2: number;
        if (b < HYPOT_TWO_MINUS_500) {
            // b < 2**-500
            if (b < 2.2250738585072014e-308) {
                // subnormal b or 0
                if (b === 0.0) return a;
                t1 = HYPOT_TWO_PLUS_1022; // t1 = 2^1022
                b *= t1;
                a *= t1;
                k -= 1022;
            } else {
                // scale a and b by 2^600
                ha = (ha + 0x25800000) | 0; // a *= 2^600
                hb = (hb + 0x25800000) | 0; // b *= 2^600
                a = a * HYPOT_TWO_PLUS_600;
                b = b * HYPOT_TWO_PLUS_600;
                k -= 600;
            }
        }
        // medium size a and b
        let w: number = a - b;
        if (w > b) {
            t1 = fdlibmWithHigh(0, ha);
            t2 = a - t1;
            w = globalThis.Math.sqrt(t1 * t1 - (b * -b - t2 * (a + t1)));
        } else {
            a = a + a;
            const y1: number = fdlibmWithHigh(0, hb);
            const y2: number = b - y1;
            t1 = fdlibmWithHigh(0, (ha + 0x00100000) | 0);
            t2 = a - t1;
            w = globalThis.Math.sqrt(t1 * y1 - (w * -w - (t1 * y2 + t2 * b)));
        }
        if (k !== 0) {
            return powerOfTwoD(k) * w;
        } else return w;
    }

    public static sqrt(a: number): number {
        return globalThis.Math.sqrt(a);
    }

    /**
    `Math.getExponent(double)`: the unbiased binary exponent of the value.
    As in Java, NaN and infinities give `Double.MAX_EXPONENT + 1` (1024), and
    zero and every subnormal give `Double.MIN_EXPONENT - 1` (-1023). The C++
    port's `std::ilogb` result for subnormals is
    `CsgExactArithmetic.cppGetExponent`.
    */
    public static getExponent(a: number): number {
        conversionFloat[0] = a;
        const high = conversionWords[highWord]!;
        return ((high >>> 20) & 0x7ff) - 1023;
    }

    /** `Math.scalb(double, int)`: `a * 2^scaleFactor` with a single rounding (`std::ldexp`). */
    public static scalb(a: number, scaleFactor: number): number {
        if (a === 0 || !globalThis.Number.isFinite(a)) {
            return a;
        }
        if (scaleFactor >= -1074 && scaleFactor <= 1023) {
            // The power of two is exact, so the product is the single rounding.
            return a * 2 ** scaleFactor;
        }
        const decomposition = Math.decompose(a);
        return Math.roundToDouble(
            decomposition.negative,
            decomposition.significand,
            decomposition.exponent + scaleFactor,
        );
    }

    /** `Math.nextAfter(double, double)` (`std::nextafter`). */
    public static nextAfter(start: number, direction: number): number {
        if (globalThis.Number.isNaN(start) || globalThis.Number.isNaN(direction)) {
            return NaN;
        }
        if (start === direction) {
            return direction;
        }
        if (start === 0) {
            return direction > 0 ? 4.9406564584124654e-324 : -4.9406564584124654e-324;
        }
        conversionFloat[0] = start;
        if (start < direction === start > 0) {
            conversionBits[0] = conversionBits[0]! + 1n;
        } else {
            conversionBits[0] = conversionBits[0]! - 1n;
        }
        return conversionFloat[0]!;
    }

    /** `Math.nextDown(double)`: the adjacent value towards negative infinity. */
    public static nextDown(d: number): number {
        return Math.nextAfter(d, globalThis.Number.NEGATIVE_INFINITY);
    }

    /** `Math.fma(double, double, double)`: `a * b + c` with a single rounding. */
    public static fma(a: number, b: number, c: number): number {
        if (!globalThis.Number.isFinite(a) || !globalThis.Number.isFinite(b)) {
            return a * b + c;
        }
        if (!globalThis.Number.isFinite(c)) {
            // The exact product of finite factors is finite.
            return c;
        }
        const x = Math.decompose(a);
        const y = Math.decompose(b);
        const z = Math.decompose(c);
        const productNegative = x.negative !== y.negative;
        let product = x.significand * y.significand;
        let productExponent = x.exponent + y.exponent;
        let addend = z.significand;
        let addendExponent = z.exponent;
        if (product === 0n) {
            productExponent = addendExponent;
        }
        if (addend === 0n) {
            addendExponent = productExponent;
        }
        const exponent = productExponent < addendExponent ? productExponent : addendExponent;
        product <<= BigInt(productExponent - exponent);
        addend <<= BigInt(addendExponent - exponent);
        const sum = (productNegative ? -product : product) + (z.negative ? -addend : addend);
        if (sum === 0n) {
            const productIsNegativeZero = productNegative;
            return productIsNegativeZero && z.negative ? -0 : 0;
        }
        return sum < 0n ? Math.roundToDouble(true, -sum, exponent) : Math.roundToDouble(false, sum, exponent);
    }

    /** Exact `value = (-1)^negative * significand * 2^exponent` of a finite double. */
    private static decompose(value: number): { negative: boolean; significand: bigint; exponent: number } {
        conversionFloat[0] = value;
        const bits = conversionBits[0]!;
        const biased = globalThis.Number((bits >> 52n) & 0x7ffn);
        const fraction = bits & 0xfffffffffffffn;
        return {
            negative: bits >> 63n === 1n,
            significand: biased === 0 ? fraction : fraction | 0x10000000000000n,
            exponent: biased === 0 ? -1074 : biased - 1075,
        };
    }

    private static bitLength(value: bigint): number {
        return value === 0n ? 0 : value.toString(2).length;
    }

    /** Rounds `significand * 2^exponent` (significand > 0) to nearest, ties to even. */
    private static roundToDouble(negative: boolean, significand: bigint, exponent: number): number {
        const length = Math.bitLength(significand);
        let quantum = exponent + length - 53;
        if (quantum < -1074) {
            quantum = -1074;
        }
        let rounded = significand;
        if (quantum > exponent) {
            const shift = BigInt(quantum - exponent);
            rounded = significand >> shift;
            const remainder = significand - (rounded << shift);
            const half = 1n << (shift - 1n);
            if (remainder > half || (remainder === half && (rounded & 1n) === 1n)) {
                rounded += 1n;
            }
        } else {
            quantum = exponent;
        }
        let result = globalThis.Number(rounded);
        // quantum >= -1074 and the value is representable (or overflows), so the
        // power-of-two steps are exact.
        while (quantum > 1000) {
            result *= 2 ** 1000;
            quantum -= 1000;
        }
        result *= 2 ** quantum;
        return negative ? -result : result;
    }
}
