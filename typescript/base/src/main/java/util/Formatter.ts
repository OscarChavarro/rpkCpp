import { OutputStream } from "../io/OutputStream";
import { String as JavaString } from "../lang/String";

export class Formatter {
  private outputStream: OutputStream | null;
  private content: JavaString;
  private closed: boolean;

  public constructor(outputStream: OutputStream | null = null) {
    this.outputStream = outputStream;
    this.content = new JavaString();
    this.closed = false;
  }

  public out(): OutputStream | null {
    return this.outputStream;
  }

  public flush(): void {
    if (this.closed || this.outputStream === null) {
      return;
    }
    this.outputStream.flush();
  }

  public close(): void {
    if (this.closed) {
      return;
    }
    this.flush();
    this.outputStream = null;
    this.closed = true;
  }

  public format(formatText: string, ...args: unknown[]): Formatter {
    if (this.closed || formatText === undefined || formatText === null) {
      return this;
    }

    const text = Formatter.vformatString(formatText, args);
    this.content = Formatter.appendText(this.content, new JavaString(text));

    if (this.outputStream !== null && text.length > 0) {
      const bytes = Buffer.from(text, "utf8");
      this.outputStream.write(bytes, 0, bytes.length);
    }
    return this;
  }

  public static format(buffer: string[], bufferSize: number, formatText: string, ...args: unknown[]): number {
    if (!Array.isArray(buffer) || bufferSize <= 0 || formatText === undefined || formatText === null) {
      return -1;
    }

    const text = Formatter.vformatString(formatText, args);
    const maxLength = bufferSize - 1;
    buffer[0] = maxLength > 0 ? text.slice(0, maxLength) : "";
    return text.length;
  }

  public static vformat(buffer: string[], bufferSize: number, formatText: string, args: unknown[]): number {
    if (!Array.isArray(buffer) || bufferSize <= 0 || formatText === undefined || formatText === null) {
      return -1;
    }

    const text = Formatter.vformatString(formatText, args);
    const maxLength = bufferSize - 1;
    buffer[0] = maxLength > 0 ? text.slice(0, maxLength) : "";
    return text.length;
  }

  private static appendText(left: JavaString, right: JavaString): JavaString {
    return new JavaString(left.toCString() + right.toCString());
  }

  private static vformatString(formatText: string, args: unknown[]): string {
    return JavaString.vformat(formatText, args);
  }
}
