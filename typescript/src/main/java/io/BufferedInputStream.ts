import { InputStream } from "./InputStream";

export class BufferedInputStream extends InputStream {
  protected inputStream: InputStream | null;

  public constructor(inputStream: InputStream | null = null) {
    super();
    this.inputStream = inputStream;
  }

  public read(): number;
  public read(buffer: Uint8Array, offset: number, length: number): number;
  public read(buffer?: Uint8Array, offset?: number, length?: number): number {
    if (this.inputStream === null) {
      return -1;
    }

    if (buffer === undefined) {
      return this.inputStream.read();
    }
    return this.inputStream.read(buffer, offset ?? 0, length ?? 0);
  }

  public close(): void {
    this.dispose();
  }

  public override dispose(): void {
    if (this.inputStream === null) {
      return;
    }
    this.inputStream.dispose();
    this.inputStream = null;
  }
}
