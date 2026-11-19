import { setWithFatalExceptions, setWithSystemExit } from "./VSDKRuntimeSettings.js";

/** Common numeric constants and utility operations in the current Java VSDK. */
export class VSDK {
    public static readonly EPSILON = 1e-6;
    public static readonly WARNING = 1;
    public static readonly ERROR = 2;
    public static readonly FATAL_ERROR = 3;
    public static readonly DEBUG = 4;
    public static readonly VERBOSE = 5;
    public static readonly POINT = 0;
    public static readonly LINE = 1;
    public static readonly TRIANGLE = 2;
    public static readonly TRIANGLE_STRIP = 3;
    public static readonly QUAD = 4;
    public static readonly QUAD_STRIP = 5;
    public static readonly PRIMITIVE_TYPE_COUNT = 6;
    public static readonly PLANE = 0;
    public static readonly SPHERE = 1;
    public static readonly CONE = 2;
    public static readonly INTERSECTION_TYPE_COUNT = 3;

    public static equals(a: number, b: number): boolean {
        return Math.abs(a - b) < VSDK.EPSILON;
    }
    public static square(a: number): number {
        return a * a;
    }
    public static formatNumberWithinZeroes(a: number | bigint, n: number): string {
        return a.toString().padStart(n, "0");
    }
    public static formatDouble(a: number): string;
    public static formatDouble(a: number, digits: number): string;
    /**
     * Java formats through `DecimalFormat`, which rounds half to even on the
     * exact binary value, keeps the sign of a negative zero or of a value that
     * rounds to zero, never switches to exponent notation, and writes the
     * non-finite values as `NaN`, `∞`, and `-∞`. JavaScript's `toFixed` rounds
     * half away from zero and gives up above 1e21, so the digits are produced
     * here with exact integer arithmetic instead.
     */
    public static formatDouble(a: number, digits = 2): string {
        if (Number.isNaN(a)) return "NaN";
        if (a === Infinity) return "∞";
        if (a === -Infinity) return "-∞";

        const decimals = digits > 0 ? digits : 0;
        const sign = a < 0 || Object.is(a, -0) ? "-" : "";
        const magnitude = Math.abs(a);

        // `toFixed` disagrees with Java only on half-way cases, and loses
        // digits once the scaled value leaves the exactly representable
        // integers, so everything else takes the cheap path.
        const scaled = magnitude * 10 ** decimals;
        if (scaled < 9007199254740992 && Math.abs(scaled - Math.floor(scaled) - 0.5) > 1e-6) {
            return sign + magnitude.toFixed(decimals);
        }

        const rounded = VSDK.roundHalfEvenScaled(magnitude, decimals).toString();
        if (decimals === 0) return sign + rounded;

        const padded = rounded.padStart(decimals + 1, "0");
        return `${sign}${padded.slice(0, -decimals)}.${padded.slice(-decimals)}`;
    }

    /**
     * `round(|value| * 10^decimals)` the way `DecimalFormat` computes it: the
     * shortest decimal representation that round-trips supplies the digits, and
     * half-way cases in those digits are decided against the exact binary value
     * before the remaining ties fall back to the half-to-even rule.
     */
    private static roundHalfEvenScaled(magnitude: number, decimals: number): bigint {
        if (magnitude === 0) return 0n;

        const [mantissaText, exponentText] = magnitude.toExponential().split("e");
        const shortestDigits = mantissaText!.replace(".", "");
        const shift = decimals + Number(exponentText) - (shortestDigits.length - 1);
        if (shift >= 0) return BigInt(shortestDigits) * 10n ** BigInt(shift);

        const divisor = 10n ** BigInt(-shift);
        const quotient = BigInt(shortestDigits) / divisor;
        const twiceRemainder = 2n * (BigInt(shortestDigits) % divisor);
        if (twiceRemainder > divisor) return quotient + 1n;
        if (twiceRemainder < divisor) return quotient;

        const exact = VSDK.compareScaledWithHalfWayPoint(magnitude, decimals, quotient);
        if (exact > 0) return quotient + 1n;
        if (exact < 0) return quotient;
        return quotient % 2n === 0n ? quotient : quotient + 1n;
    }

    /**
     * Compares the exact binary value of `magnitude * 10^decimals` against the
     * half-way point `quotient + 1/2`, with no intermediate rounding.
     */
    private static compareScaledWithHalfWayPoint(magnitude: number, decimals: number, quotient: bigint): number {
        const bits = new DataView(new ArrayBuffer(8));
        bits.setFloat64(0, magnitude, false);
        const raw = bits.getBigUint64(0, false);
        const biasedExponent = Number((raw >> 52n) & 0x7ffn);
        const storedMantissa = raw & 0xf_ffff_ffff_ffffn;
        const mantissa = biasedExponent === 0 ? storedMantissa : storedMantissa | 0x10_0000_0000_0000n;
        const exponent = (biasedExponent === 0 ? 1 : biasedExponent) - 1075;

        let numerator = 2n * mantissa * 10n ** BigInt(decimals);
        let denominator = 1n;
        if (exponent >= 0) numerator <<= BigInt(exponent);
        else denominator = 1n << BigInt(-exponent);

        const halfWayPoint = (2n * quotient + 1n) * denominator;
        return numerator > halfWayPoint ? 1 : numerator < halfWayPoint ? -1 : 0;
    }
    public static formatByteAsHex(a: number): string {
        const i = VSDK.signedByte2unsignedInteger(a);
        return i.toString(16).padStart(2, "0").toUpperCase();
    }
    public static formatIntAsHex(a: number): string {
        return (a >>> 0).toString(16).padStart(8, "0").toUpperCase();
    }
    public static signedByte2unsignedInteger(input: number): number {
        return input < 0 ? input + 256 : input;
    }
    public static unsigned8BitInteger2signedByte(input: number): number {
        const value = Math.min(255, Math.max(0, input));
        return value > 127 ? value - 256 : value;
    }
    public static setWithSystemExit(flag: boolean): void {
        setWithSystemExit(flag);
    }
    public static setWithFatalExceptions(flag: boolean): void {
        setWithFatalExceptions(flag);
    }
}
