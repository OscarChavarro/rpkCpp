import { String as JavaString } from "../lang/String";

export class StringTokenizer {
  private readonly text: string;
  private readonly delimiters: string;
  private cursor: number;

  public constructor(text: JavaString, delimiters = " \t\n\r\f") {
    this.text = text.toCString();
    this.delimiters = delimiters ?? " \t\n\r\f";
    this.cursor = 0;
  }

  public dispose(): void {
    this.cursor = this.text.length;
  }

  private isDelimiter(ch: string): boolean {
    return this.delimiters.indexOf(ch) >= 0;
  }

  private findTokenStart(from: number): number {
    let index = from < 0 ? 0 : from;
    while (index < this.text.length && this.isDelimiter(this.text.charAt(index))) {
      index++;
    }
    if (index >= this.text.length) {
      return -1;
    }
    return index;
  }

  private findTokenEnd(from: number): number {
    let index = from < 0 ? 0 : from;
    while (index < this.text.length && !this.isDelimiter(this.text.charAt(index))) {
      index++;
    }
    if (index >= this.text.length) {
      return -1;
    }
    return index;
  }

  public hasMoreTokens(): boolean {
    return this.findTokenStart(this.cursor) >= 0;
  }

  public nextToken(): JavaString {
    const tokenStart = this.findTokenStart(this.cursor);
    if (tokenStart < 0) {
      return new JavaString();
    }

    const tokenEnd = this.findTokenEnd(tokenStart);
    const effectiveTokenEnd = tokenEnd < 0 ? this.text.length : tokenEnd;
    const token = this.text.slice(tokenStart, effectiveTokenEnd);
    this.cursor = tokenEnd < 0 ? this.text.length : tokenEnd + 1;
    return new JavaString(token);
  }
}
