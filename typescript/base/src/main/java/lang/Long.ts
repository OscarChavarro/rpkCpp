import { IllegalArgumentException } from "./IllegalArgumentException.js";

/** Static Java long operations represented exactly by bigint. */
export class Long {
    public static readonly MIN_VALUE = -9_223_372_036_854_775_808n;
    public static readonly MAX_VALUE = 9_223_372_036_854_775_807n;
    public static readonly SIZE = 64;
    public static readonly BYTES = 8;

    public static valueOf(value: bigint | string, radix = 10): bigint {
        return typeof value === "bigint" ? Long.toLong(value) : Long.parseLong(value, radix);
    }
    public static parseLong(text: string, radix = 10): bigint {
        const value = Long.parse(text, radix, true);
        if (value < Long.MIN_VALUE || value > Long.MAX_VALUE)
            throw new IllegalArgumentException(`For input string: "${text}"`);
        return value;
    }
    public static parseUnsignedLong(text: string, radix = 10): bigint {
        const value = Long.parse(text, radix, false);
        if (value > 0xffff_ffff_ffff_ffffn) throw new IllegalArgumentException(`For input string: "${text}"`);
        return BigInt.asIntN(64, value);
    }
    public static toString(value: bigint, radix = 10): string {
        Long.requireRadix(radix);
        return Long.toLong(value).toString(radix);
    }
    public static toUnsignedString(value: bigint, radix = 10): string {
        Long.requireRadix(radix);
        return BigInt.asUintN(64, value).toString(radix);
    }
    public static toHexString(value: bigint): string {
        return Long.toUnsignedString(value, 16);
    }
    public static toOctalString(value: bigint): string {
        return Long.toUnsignedString(value, 8);
    }
    public static toBinaryString(value: bigint): string {
        return Long.toUnsignedString(value, 2);
    }
    public static compare(left: bigint, right: bigint): number {
        return left < right ? -1 : left > right ? 1 : 0;
    }
    public static compareUnsigned(left: bigint, right: bigint): number {
        const a = BigInt.asUintN(64, left);
        const b = BigInt.asUintN(64, right);
        return a < b ? -1 : a > b ? 1 : 0;
    }
    public static hashCode(value: bigint): number {
        const bits = BigInt.asUintN(64, value);
        return Number((bits ^ (bits >> 32n)) & 0xffff_ffffn) | 0;
    }
    public static toLong(value: bigint): bigint {
        return BigInt.asIntN(64, value);
    }
    public static reverseBytes(value: bigint): bigint {
        let bits = BigInt.asUintN(64, value);
        let reversed = 0n;
        for (let i = 0; i < 8; i++) {
            reversed = (reversed << 8n) | (bits & 0xffn);
            bits >>= 8n;
        }
        return BigInt.asIntN(64, reversed);
    }
    public static numberOfLeadingZeros(value: bigint): number {
        let bits = BigInt.asUintN(64, value);
        if (bits === 0n) return 64;
        let count = 0;
        while ((bits & (1n << 63n)) === 0n) {
            count++;
            bits <<= 1n;
        }
        return count;
    }
    public static numberOfTrailingZeros(value: bigint): number {
        let bits = BigInt.asUintN(64, value);
        if (bits === 0n) return 64;
        let count = 0;
        while ((bits & 1n) === 0n) {
            count++;
            bits >>= 1n;
        }
        return count;
    }
    public static toInt(value: bigint): number {
        return Number(BigInt.asIntN(32, value));
    }

    private static parse(text: string, radix: number, signed: boolean): bigint {
        Long.requireRadix(radix);
        if (text.length === 0) throw new IllegalArgumentException('For input string: ""');
        const negative = text[0] === "-";
        const signedPrefix = negative || text[0] === "+";
        const digits = signedPrefix ? text.slice(1) : text;
        if (digits.length === 0 || (!signed && signedPrefix))
            throw new IllegalArgumentException(`For input string: "${text}"`);
        let result = 0n;
        for (const character of digits) {
            const code = character.codePointAt(0) ?? -1;
            const digit =
                code >= 48 && code <= 57
                    ? code - 48
                    : code >= 65 && code <= 90
                      ? code - 65 + 10
                      : code >= 97 && code <= 122
                        ? code - 97 + 10
                        : -1;
            if (digit < 0 || digit >= radix) throw new IllegalArgumentException(`For input string: "${text}"`);
            result = result * BigInt(radix) + BigInt(digit);
        }
        return negative ? -result : result;
    }
    private static requireRadix(radix: number): void {
        if (!Number.isInteger(radix) || radix < 2 || radix > 36)
            throw new IllegalArgumentException(`radix ${radix} out of range`);
    }
}
