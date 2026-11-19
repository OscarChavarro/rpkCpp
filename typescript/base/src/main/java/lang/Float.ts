import { IllegalArgumentException } from "./IllegalArgumentException.js";

/** Java float helpers. Every returned float value is rounded to IEEE-754 binary32. */
export class Float {
    public static readonly MIN_VALUE = 1.401298464324817e-45;
    public static readonly MIN_NORMAL = 1.1754943508222875e-38;
    public static readonly MAX_VALUE = 3.4028234663852886e38;
    public static readonly POSITIVE_INFINITY = Infinity;
    public static readonly NEGATIVE_INFINITY = -Infinity;
    public static readonly NaN = NaN;
    public static readonly SIZE = 32;
    public static readonly BYTES = 4;

    public static valueOf(value: number | string): number {
        return typeof value === "number" ? Math.fround(value) : Float.parseFloat(value);
    }
    public static parseFloat(text: string): number {
        if (text.length === 0 || text.trim() !== text)
            throw new IllegalArgumentException(`For input string: "${text}"`);
        if (text === "NaN") return NaN;
        if (text === "Infinity" || text === "+Infinity") return Infinity;
        if (text === "-Infinity") return -Infinity;
        const value = Number(
            text.endsWith("f") || text.endsWith("F") || text.endsWith("d") || text.endsWith("D")
                ? text.slice(0, -1)
                : text,
        );
        if (Number.isNaN(value)) throw new IllegalArgumentException(`For input string: "${text}"`);
        return Math.fround(value);
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
        const a = Float.floatToIntBits(left);
        const b = Float.floatToIntBits(right);
        return a === b ? 0 : a < b ? -1 : 1;
    }
    public static floatToRawIntBits(value: number): number {
        const view = new DataView(new ArrayBuffer(4));
        view.setFloat32(0, value, false);
        return view.getInt32(0, false);
    }
    public static floatToIntBits(value: number): number {
        return Number.isNaN(value) ? 0x7fc0_0000 : Float.floatToRawIntBits(value);
    }
    public static intBitsToFloat(bits: number): number {
        const view = new DataView(new ArrayBuffer(4));
        view.setInt32(0, bits | 0, false);
        return view.getFloat32(0, false);
    }
    public static hashCode(value: number): number {
        return Float.floatToIntBits(value);
    }
    public static toString(value: number): string {
        return Number.isNaN(value)
            ? "NaN"
            : value === Infinity
              ? "Infinity"
              : value === -Infinity
                ? "-Infinity"
                : String(Math.fround(value));
    }
}
