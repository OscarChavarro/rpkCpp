import { IllegalArgumentException } from "./IllegalArgumentException.js";

/** Static Java int operations; every result is normalized to signed 32 bits. */
export class Integer {
    public static readonly MIN_VALUE = -2_147_483_648;
    public static readonly MAX_VALUE = 2_147_483_647;
    public static readonly SIZE = 32;
    public static readonly BYTES = 4;

    public static valueOf(value: number | string, radix = 10): number {
        return typeof value === "number" ? Integer.toInt(value) : Integer.parseInt(value, radix);
    }
    public static parseInt(text: string, radix = 10): number {
        const value = Integer.parse(text, radix, true);
        if (value < BigInt(Integer.MIN_VALUE) || value > BigInt(Integer.MAX_VALUE))
            throw new IllegalArgumentException(`For input string: "${text}"`);
        return Number(value);
    }
    public static parseUnsignedInt(text: string, radix = 10): number {
        const value = Integer.parse(text, radix, false);
        if (value > 0xffff_ffffn) throw new IllegalArgumentException(`For input string: "${text}"`);
        return Number(value) | 0;
    }
    public static toString(value: number, radix = 10): string {
        Integer.requireRadix(radix);
        return Integer.toInt(value).toString(radix);
    }
    public static toUnsignedString(value: number, radix = 10): string {
        Integer.requireRadix(radix);
        return BigInt.asUintN(32, BigInt(Integer.toInt(value))).toString(radix);
    }
    public static toHexString(value: number): string {
        return Integer.toUnsignedString(value, 16);
    }
    public static toOctalString(value: number): string {
        return Integer.toUnsignedString(value, 8);
    }
    public static toBinaryString(value: number): string {
        return Integer.toUnsignedString(value, 2);
    }
    public static compare(left: number, right: number): number {
        const a = Integer.toInt(left);
        const b = Integer.toInt(right);
        return a < b ? -1 : a > b ? 1 : 0;
    }
    public static compareUnsigned(left: number, right: number): number {
        const a = left >>> 0;
        const b = right >>> 0;
        return a < b ? -1 : a > b ? 1 : 0;
    }
    public static toUnsignedLong(value: number): bigint {
        return BigInt(value >>> 0);
    }
    public static numberOfLeadingZeros(value: number): number {
        return Math.clz32(value);
    }
    public static numberOfTrailingZeros(value: number): number {
        const normalized = value | 0;
        return normalized === 0 ? 32 : 31 - Math.clz32(normalized & -normalized);
    }
    public static bitCount(value: number): number {
        let bits = value >>> 0;
        bits -= (bits >>> 1) & 0x5555_5555;
        bits = (bits & 0x3333_3333) + ((bits >>> 2) & 0x3333_3333);
        return (((bits + (bits >>> 4)) & 0x0f0f_0f0f) * 0x0101_0101) >>> 24;
    }
    public static rotateLeft(value: number, distance: number): number {
        const shift = distance & 31;
        return (value << shift) | (value >>> ((32 - shift) & 31));
    }
    public static rotateRight(value: number, distance: number): number {
        const shift = distance & 31;
        return (value >>> shift) | (value << ((32 - shift) & 31));
    }
    public static reverseBytes(value: number): number {
        const bits = value >>> 0;
        return ((bits & 0xff) << 24) | ((bits & 0xff00) << 8) | ((bits >>> 8) & 0xff00) | (bits >>> 24) | 0;
    }
    public static hashCode(value: number): number {
        return Integer.toInt(value);
    }
    public static toInt(value: number): number {
        return value | 0;
    }

    private static parse(text: string, radix: number, signed: boolean): bigint {
        Integer.requireRadix(radix);
        if (text.length === 0) throw new IllegalArgumentException('For input string: ""');
        const negative = text[0] === "-";
        const signedPrefix = negative || text[0] === "+";
        const digits = signedPrefix ? text.slice(1) : text;
        if (digits.length === 0 || (!signed && signedPrefix))
            throw new IllegalArgumentException(`For input string: "${text}"`);
        let result = 0n;
        for (const character of digits) {
            const digit = Integer.digit(character);
            if (digit < 0 || digit >= radix) throw new IllegalArgumentException(`For input string: "${text}"`);
            result = result * BigInt(radix) + BigInt(digit);
        }
        return negative ? -result : result;
    }
    private static digit(character: string): number {
        const code = character.codePointAt(0) ?? -1;
        if (code >= 48 && code <= 57) return code - 48;
        if (code >= 65 && code <= 90) return code - 65 + 10;
        if (code >= 97 && code <= 122) return code - 97 + 10;
        return -1;
    }
    private static requireRadix(radix: number): void {
        if (!Number.isInteger(radix) || radix < 2 || radix > 36)
            throw new IllegalArgumentException(`radix ${radix} out of range`);
    }
}
