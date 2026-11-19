// Shared binary64 bit views (single-threaded per realm).
const conversionBuffer = new ArrayBuffer(8);
const conversionFloat = new Float64Array(conversionBuffer);
const conversionBits = new BigUint64Array(conversionBuffer);
const conversionWords = new Uint32Array(conversionBuffer);
conversionFloat[0] = 1.0;
const highWord = conversionWords[1] === 0x3ff00000 ? 1 : 0;
const lowWord = 1 - highWord;

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

    public static min(a: number, b: number): number {
        return a < b ? a : b;
    }

    public static max(a: number, b: number): number {
        return a > b ? a : b;
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

    public static sqrt(a: number): number {
        return globalThis.Math.sqrt(a);
    }

    /**
    Unbiased binary exponent, as the C++ emulation (`std::ilogb`) computes it:
    subnormal values report their true exponent (`-1074` for `MIN_VALUE`).
    Zero, infinities and NaN keep the historical result `0` of this port.
    */
    public static getExponent(a: number): number {
        if (a === 0 || !globalThis.Number.isFinite(a)) {
            return 0;
        }
        conversionFloat[0] = a;
        const high = conversionWords[highWord]!;
        const biased = (high >>> 20) & 0x7ff;
        if (biased !== 0) {
            return biased - 1023;
        }
        // Subnormal: the exponent of the leading fraction bit.
        const highFraction = high & 0xfffff;
        if (highFraction !== 0) {
            return -1011 - globalThis.Math.clz32(highFraction);
        }
        return -1043 - globalThis.Math.clz32(conversionWords[lowWord]!);
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
