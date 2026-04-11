export class TokenValidationContext {
  public static isIntDelimited(text: string, delimiters: string): boolean {
    const cp = TokenValidationContext.skipInt(text);
    if (cp < 0) {
      return false;
    }
    if (cp === text.length) {
      return true;
    }
    return delimiters.indexOf(text.charAt(cp)) >= 0;
  }

  public static isFloatDelimited(text: string, delimiters: string): boolean {
    const cp = TokenValidationContext.skipFloat(text);
    if (cp < 0) {
      return false;
    }
    if (cp === text.length) {
      return true;
    }
    return delimiters.indexOf(text.charAt(cp)) >= 0;
  }

  public static isFloat(text: string): boolean {
    const cp = TokenValidationContext.skipFloat(text);
    return cp >= 0 && cp === text.length;
  }

  public static isInt(text: string): boolean {
    const cp = TokenValidationContext.skipInt(text);
    return cp >= 0 && cp === text.length;
  }

  public static isName(text: string): boolean {
    let index = 0;
    while (index < text.length && text.charAt(index) === "_") {
      index++;
    }
    if (index >= text.length || !TokenValidationContext.isAsciiCode(text.charCodeAt(index)) || !/[A-Za-z]/.test(text.charAt(index))) {
      return false;
    }
    let tokenIndex = index + 1;
    while (
      tokenIndex < text.length
      && TokenValidationContext.isAsciiCode(text.charCodeAt(tokenIndex))
      && TokenValidationContext.isAsciiGraph(text.charCodeAt(tokenIndex))
    ) {
      tokenIndex++;
    }
    return tokenIndex === text.length;
  }

  private static isAsciiCode(value: number): boolean {
    return value >= 0 && value <= 127;
  }

  private static isAsciiGraph(value: number): boolean {
    return value >= 33 && value <= 126;
  }

  private static skipInt(text: string): number {
    return TokenValidationContext.skipIntFrom(text, 0);
  }

  private static skipIntFrom(text: string, start: number): number {
    let index = start;
    while (index < text.length && /\s/.test(text.charAt(index))) {
      index++;
    }
    if (index < text.length && (text.charAt(index) === "-" || text.charAt(index) === "+")) {
      index++;
    }
    if (index >= text.length || !/[0-9]/.test(text.charAt(index))) {
      return -1;
    }
    do {
      index++;
    } while (index < text.length && /[0-9]/.test(text.charAt(index)));
    return index;
  }

  private static skipFloat(text: string): number {
    let startIndex = 0;
    while (startIndex < text.length && /\s/.test(text.charAt(startIndex))) {
      startIndex++;
    }
    if (startIndex < text.length && (text.charAt(startIndex) === "-" || text.charAt(startIndex) === "+")) {
      startIndex++;
    }
    let currentIndex = startIndex;
    while (currentIndex < text.length && /[0-9]/.test(text.charAt(currentIndex))) {
      currentIndex++;
    }
    if (currentIndex < text.length && text.charAt(currentIndex) === ".") {
      currentIndex++;
      startIndex++;
      while (currentIndex < text.length && /[0-9]/.test(text.charAt(currentIndex))) {
        currentIndex++;
      }
    }
    if (currentIndex === startIndex) {
      return -1;
    }
    if (currentIndex < text.length && (text.charAt(currentIndex) === "e" || text.charAt(currentIndex) === "E")) {
      return TokenValidationContext.skipIntFrom(text, currentIndex + 1);
    }
    return currentIndex;
  }
}
