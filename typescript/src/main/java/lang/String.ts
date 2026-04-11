const util = require("node:util");

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
    return new String(util.format(formatText, ...args));
  }

  public toString(): string {
    return this.value;
  }
}
