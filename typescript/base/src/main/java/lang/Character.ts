export class Character {
    private static toCodePoint(value: number | string): number {
        if (typeof value === "number") {
            return value;
        }
        if (!value || value.length === 0) {
            return 0;
        }
        return value.charCodeAt(0);
    }

    public static isDigit(value: number | string): boolean {
        const code = Character.toCodePoint(value);
        return code >= 48 && code <= 57;
    }

    public static isSpace(value: number | string): boolean {
        const code = Character.toCodePoint(value);
        return code === 32 || (code >= 9 && code <= 13);
    }

    /**
    Java `Character.isWhitespace`: a Unicode space, line or paragraph separator
    that is not a non-breaking space (U+00A0, U+2007, U+202F), or one of
    U+0009 - U+000D and U+001C - U+001F.
    */
    public static isWhitespace(value: number | string): boolean {
        const code = Character.toCodePoint(value);
        if ((code >= 9 && code <= 13) || (code >= 28 && code <= 31)) {
            return true;
        }
        if (code === 0xa0 || code === 0x2007 || code === 0x202f) {
            return false;
        }
        return /^[\p{Zs}\p{Zl}\p{Zp}]$/u.test(globalThis.String.fromCharCode(code));
    }

    public static isLetter(value: number | string): boolean {
        const code = Character.toCodePoint(value);
        return (code >= 65 && code <= 90) || (code >= 97 && code <= 122);
    }
}
