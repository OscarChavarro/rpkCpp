import { OutputStream } from "./OutputStream";
import { String as JavaString } from "../lang/String";

export class PrintStream {
  private readonly stream: OutputStream | null;

  public constructor(stream: OutputStream | null) {
    this.stream = stream;
  }

  private static writeText(stream: OutputStream | null, text: string | null): void {
    if (stream === null || text === null || text.length === 0) {
      return;
    }

    const bytes = Buffer.from(text, "utf8");
    stream.write(bytes, 0, bytes.length);
  }

  public printf(formatText: string, ...args: unknown[]): PrintStream {
    if (this.stream === null || formatText === undefined || formatText === null) {
      return this;
    }
    PrintStream.writeText(this.stream, JavaString.vformat(formatText, args));
    return this;
  }

  public print(text: string): void {
    PrintStream.writeText(this.stream, text);
  }

  public println(text?: string): void {
    if (this.stream === null) {
      return;
    }
    if (text !== undefined) {
      PrintStream.writeText(this.stream, text);
    }
    this.stream.write(10);
  }

  public flush(): void {
    if (this.stream === null) {
      return;
    }
    this.stream.flush();
  }
}
