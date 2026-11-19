import { IllegalArgumentException } from "./IllegalArgumentException.js";

/** Java double helpers, including lossless IEEE-754 bit conversions. */
export class Double {
    public static readonly MIN_VALUE = 4.9406564584124654e-324;
    public static readonly MIN_NORMAL = 2.2250738585072014e-308;
    public static readonly MAX_VALUE = 1.7976931348623157e308;
    public static readonly POSITIVE_INFINITY = Infinity;
    public static readonly NEGATIVE_INFINITY = -Infinity;
    public static readonly NaN = NaN;
    public static readonly SIZE = 64;
    public static readonly BYTES = 8;

    public static valueOf(value: number | string): number {
        return typeof value === "number" ? value : Double.parseDouble(value);
    }
    public static parseDouble(input: string): number {
        // Java trims the leading and trailing whitespace (`String.trim`: code
        // points up to U+0020) before parsing
        // eslint-disable-next-line no-control-regex
        const text = input.replace(/^[\u0000-\u0020]+|[\u0000-\u0020]+$/g, "");
        if (text.length === 0) throw new IllegalArgumentException(`For input string: "${input}"`);
        if (text === "NaN") return NaN;
        if (text === "Infinity" || text === "+Infinity") return Infinity;
        if (text === "-Infinity") return -Infinity;
        const value = Number(
            text.endsWith("d") || text.endsWith("D") || text.endsWith("f") || text.endsWith("F")
                ? text.slice(0, -1)
                : text,
        );
        if (Number.isNaN(value)) throw new IllegalArgumentException(`For input string: "${text}"`);
        return value;
    }
    public static isNaN(value: number): boolean {
        return Number.isNaN(value);
    }
    public static isInfinite(value: number): boolean {
        return value === Infinity || value === -Infinity;
    }
    public static isFinite(value: number): boolean {
        return Number.isFinite(value);
    }
    public static compare(left: number, right: number): number {
        if (left < right) return -1;
        if (left > right) return 1;
        const a = Double.doubleToLongBits(left);
        const b = Double.doubleToLongBits(right);
        return a === b ? 0 : a < b ? -1 : 1;
    }
    public static doubleToRawLongBits(value: number): bigint {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, false);
        return view.getBigInt64(0, false);
    }
    public static doubleToLongBits(value: number): bigint {
        return Number.isNaN(value) ? 0x7ff8_0000_0000_0000n : Double.doubleToRawLongBits(value);
    }
    public static longBitsToDouble(bits: bigint): number {
        const view = new DataView(new ArrayBuffer(8));
        view.setBigInt64(0, BigInt.asIntN(64, bits), false);
        return view.getFloat64(0, false);
    }
    public static hashCode(value: number): number {
        const bits = BigInt.asUintN(64, Double.doubleToLongBits(value));
        return Number((bits ^ (bits >> 32n)) & 0xffff_ffffn) | 0;
    }
    /**
     * Java's `Double.toString` layout: a plain decimal with at least one
     * fraction digit when the magnitude is in [1e-3, 1e7), and the
     * `<digit>.<digits>E<exponent>` computerized scientific form otherwise.
     * JavaScript instead drops the trailing `.0`, writes `1e-7` in lower case,
     * and keeps plain notation up to 1e21.
     *
     * The significant digits are the shortest ones that round-trip, which is
     * what both runtimes compute; the pre-19 Java implementation emits one
     * extra digit for a small set of values, and that residual difference is
     * not reproduced here.
     */
    public static toString(value: number): string {
        if (Number.isNaN(value)) return "NaN";
        if (value === Infinity) return "Infinity";
        if (value === -Infinity) return "-Infinity";
        if (value === 0) return Object.is(value, -0) ? "-0.0" : "0.0";

        const sign = value < 0 ? "-" : "";
        const magnitude = Math.abs(value);
        const exponential = magnitude.toExponential();
        const [mantissa, exponentText] = exponential.split("e");
        const exponent = Number(exponentText);
        const digits = mantissa!.replace(".", "");

        if (magnitude >= 1e-3 && magnitude < 1e7) {
            if (exponent >= 0) {
                const integerPart = digits.slice(0, exponent + 1).padEnd(exponent + 1, "0");
                const fractionPart = digits.slice(exponent + 1);
                return `${sign}${integerPart}.${fractionPart === "" ? "0" : fractionPart}`;
            }
            return `${sign}0.${"0".repeat(-exponent - 1)}${digits}`;
        }
        const fractionPart = digits.slice(1);
        return `${sign}${digits.charAt(0)}.${fractionPart === "" ? "0" : fractionPart}E${exponent}`;
    }
}
