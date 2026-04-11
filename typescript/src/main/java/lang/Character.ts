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

  public static isLetter(value: number | string): boolean {
    const code = Character.toCodePoint(value);
    return (code >= 65 && code <= 90) || (code >= 97 && code <= 122);
  }
}
