export class String {
  private value: string;

  public constructor(text?: String | string | null) {
    if (text instanceof String) {
      this.value = text.toCString();
      return;
    }
    if (typeof text === "string") {
      this.value = text;
      return;
    }
    this.value = "";
  }

  public dispose(): void {
    this.value = "";
  }

  public assign(other: String): String {
    this.value = other.toCString();
    return this;
  }

  public toCString(): string {
    return this.value;
  }

  public length(): number {
    return this.value.length;
  }

  public isEmpty(): boolean {
    return this.value.length === 0;
  }

  public charAt(index: number): string {
    if (index < 0 || index >= this.value.length) {
      return "\0";
    }
    return this.value.charAt(index);
  }

  public equals(other: String | string | null): boolean {
    if (other === null) {
      return this.value === "";
    }
    if (other instanceof String) {
      return this.value === other.toCString();
    }
    return this.value === other;
  }

  public substring(beginIndex: number): String;
  public substring(beginIndex: number, endIndex: number): String;
  public substring(beginIndex: number, endIndex?: number): String {
    const sourceLength = this.length();
    const safeBegin = beginIndex < 0 ? 0 : beginIndex;
    const safeEnd = endIndex === undefined
      ? sourceLength
      : (endIndex > sourceLength ? sourceLength : endIndex);

    if (safeBegin >= sourceLength || safeEnd <= safeBegin) {
      return new String();
    }

    return new String(this.value.slice(safeBegin, safeEnd));
  }

  public indexOf(token: string | number, fromIndex = 0): number {
    const safeFromIndex = fromIndex < 0 ? 0 : fromIndex;
    if (safeFromIndex >= this.value.length) {
      return -1;
    }

    const searchToken = typeof token === "number"
      ? globalThis.String.fromCharCode(token)
      : token;
    return this.value.indexOf(searchToken, safeFromIndex);
  }

  public startsWith(prefix: string | null): boolean {
    if (prefix === null) {
      return false;
    }
    return this.value.startsWith(prefix);
  }

  public static formatCStringToJavaString(formatText: string | null, ...args: unknown[]): String {
    if (formatText === null) {
      return new String();
    }
    return new String(String.vformat(formatText, args));
  }

  private static argToText(arg: unknown): string {
    if (arg === null) {
      return "null";
    }
    if (arg === undefined) {
      return "undefined";
    }
    return globalThis.String(arg);
  }

  private static exponentialText(value: number, fractionDigits: number): string {
    const text = value.toExponential(fractionDigits);
    return text.replace(/e([+-])(\d)$/, "e$10$2");
  }

  private static generalFloatText(value: number, precision: number): string {
    if (!globalThis.Number.isFinite(value)) {
      return globalThis.String(value);
    }
    const significantDigits = precision === 0 ? 1 : precision;
    const magnitude = globalThis.Math.abs(value);
    if (magnitude === 0) {
      return (0).toFixed(significantDigits - 1);
    }
    if (magnitude >= 1e-4 && magnitude < globalThis.Math.pow(10, significantDigits)) {
      return value.toPrecision(significantDigits);
    }
    return String.exponentialText(value, significantDigits - 1);
  }

  private static padText(text: string, flags: string, width: number): string {
    if (text.length >= width) {
      return text;
    }
    if (flags.indexOf("-") >= 0) {
      return text.padEnd(width, " ");
    }
    if (flags.indexOf("0") >= 0) {
      const signLength = text.startsWith("-") || text.startsWith("+") ? 1 : 0;
      return text.slice(0, signLength) + text.slice(signLength).padStart(width - signLength, "0");
    }
    return text.padStart(width, " ");
  }

  private static signText(text: string, value: number, flags: string): string {
    if (value >= 0 || globalThis.Number.isNaN(value)) {
      if (flags.indexOf("+") >= 0) {
        return "+" + text;
      }
      if (flags.indexOf(" ") >= 0) {
        return " " + text;
      }
    }
    return text;
  }

  public static vformat(formatText: string, args: unknown[]): string {
    let result = "";
    let argIndex = 0;
    let i = 0;
    const n = formatText.length;

    while (i < n) {
      const c = formatText.charAt(i);
      if (c !== "%") {
        result += c;
        i++;
        continue;
      }

      let j = i + 1;
      let flags = "";
      while (j < n && "-#+ 0,(".indexOf(formatText.charAt(j)) >= 0) {
        flags += formatText.charAt(j);
        j++;
      }

      let widthText = "";
      while (j < n && formatText.charAt(j) >= "0" && formatText.charAt(j) <= "9") {
        widthText += formatText.charAt(j);
        j++;
      }

      let hasPrecision = false;
      let precisionText = "";
      if (j < n && formatText.charAt(j) === ".") {
        hasPrecision = true;
        j++;
        while (j < n && formatText.charAt(j) >= "0" && formatText.charAt(j) <= "9") {
          precisionText += formatText.charAt(j);
          j++;
        }
      }

      if (j >= n) {
        result += formatText.slice(i);
        break;
      }

      const conversion = formatText.charAt(j);
      const width = widthText.length > 0 ? parseInt(widthText, 10) : 0;
      const precision = precisionText.length > 0 ? parseInt(precisionText, 10) : 0;
      i = j + 1;

      if (conversion === "%") {
        result += "%";
        continue;
      }
      if (conversion === "n") {
        result += "\n";
        continue;
      }

      const arg = args[argIndex];
      argIndex++;
      let text: string;

      switch (conversion) {
        case "d": {
          const value = globalThis.Math.trunc(globalThis.Number(arg));
          text = String.signText(globalThis.String(value), value, flags);
          break;
        }
        case "o": {
          text = globalThis.Math.trunc(globalThis.Number(arg)).toString(8);
          break;
        }
        case "x":
        case "X": {
          text = globalThis.Math.trunc(globalThis.Number(arg)).toString(16);
          if (conversion === "X") {
            text = text.toUpperCase();
          }
          break;
        }
        case "f": {
          const value = globalThis.Number(arg);
          const fractionDigits = hasPrecision ? precision : 6;
          text = globalThis.Number.isFinite(value)
            ? String.signText(value.toFixed(fractionDigits), value, flags)
            : globalThis.String(value);
          break;
        }
        case "e":
        case "E": {
          const value = globalThis.Number(arg);
          const fractionDigits = hasPrecision ? precision : 6;
          text = globalThis.Number.isFinite(value)
            ? String.signText(String.exponentialText(value, fractionDigits), value, flags)
            : globalThis.String(value);
          if (conversion === "E") {
            text = text.toUpperCase();
          }
          break;
        }
        case "g":
        case "G": {
          const value = globalThis.Number(arg);
          text = String.signText(String.generalFloatText(value, hasPrecision ? precision : 6), value, flags);
          if (conversion === "G") {
            text = text.toUpperCase();
          }
          break;
        }
        case "c": {
          text = typeof arg === "number"
            ? globalThis.String.fromCharCode(arg)
            : String.argToText(arg).charAt(0);
          break;
        }
        case "b":
        case "B": {
          text = arg === null || arg === undefined || arg === false ? "false" : "true";
          if (conversion === "B") {
            text = text.toUpperCase();
          }
          break;
        }
        case "s":
        case "S": {
          text = String.argToText(arg);
          if (hasPrecision) {
            text = text.slice(0, precision);
          }
          if (conversion === "S") {
            text = text.toUpperCase();
          }
          break;
        }
        default: {
          text = formatText.slice(i - 1 - flags.length - widthText.length
            - (hasPrecision ? precisionText.length + 1 : 0) - 1, i);
          argIndex--;
          break;
        }
      }

      result += String.padText(text, flags, width);
    }

    return result;
  }

  public toString(): string {
    return this.value;
  }
}
