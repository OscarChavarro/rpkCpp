import { InputStream } from "./InputStream";

const fs = require("node:fs");

export class FileInputStream extends InputStream {
  private fileDescriptor: number | null;
  private position: number;

  public constructor(fileName: string) {
    super();
    this.fileDescriptor = null;
    this.position = 0;
    if (fileName && fileName.length > 0) {
      try {
        this.fileDescriptor = fs.openSync(fileName, "r");
      }
      catch (_error) {
        this.fileDescriptor = null;
      }
    }
  }

  public read(): number;
  public read(buffer: Uint8Array, offset: number, length: number): number;
  public read(buffer?: Uint8Array, offset?: number, length?: number): number {
    if (this.fileDescriptor === null) {
      return -1;
    }

    if (buffer === undefined) {
      const localBuffer = new Uint8Array(1);
      const readCount = fs.readSync(this.fileDescriptor, localBuffer, 0, 1, this.position);
      if (readCount <= 0) {
        return -1;
      }
      this.position += readCount;
      return localBuffer[0];
    }

    const safeOffset = offset ?? 0;
    const safeLength = length ?? 0;
    if (safeOffset < 0 || safeLength < 0 || safeOffset + safeLength > buffer.length) {
      return -1;
    }
    if (safeLength === 0) {
      return 0;
    }

    const readCount = fs.readSync(this.fileDescriptor, buffer, safeOffset, safeLength, this.position);
    if (readCount <= 0) {
      return -1;
    }
    this.position += readCount;
    return readCount;
  }

  public close(): void {
    if (this.fileDescriptor === null) {
      return;
    }
    fs.closeSync(this.fileDescriptor);
    this.fileDescriptor = null;
  }

  public override dispose(): void {
    this.close();
  }
}
