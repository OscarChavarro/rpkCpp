import { OutputStream } from "./OutputStream";

const fs = require("node:fs");

export class FileOutputStream extends OutputStream {
  private fileDescriptor: number | null;

  public constructor(fileName: string) {
    super();
    this.fileDescriptor = null;
    if (fileName && fileName.length > 0) {
      try {
        this.fileDescriptor = fs.openSync(fileName, "w");
      }
      catch (_error) {
        this.fileDescriptor = null;
      }
    }
  }

  public write(value: number): void;
  public write(buffer: Uint8Array, offset: number, length: number): void;
  public write(valueOrBuffer: number | Uint8Array, offset?: number, length?: number): void {
    if (this.fileDescriptor === null) {
      return;
    }

    if (typeof valueOrBuffer === "number") {
      const oneByte = new Uint8Array([valueOrBuffer & 0xFF]);
      fs.writeSync(this.fileDescriptor, oneByte, 0, 1);
      return;
    }

    const safeOffset = offset ?? 0;
    const safeLength = length ?? 0;
    if (safeOffset < 0 || safeLength < 0 || safeOffset + safeLength > valueOrBuffer.length) {
      return;
    }
    if (safeLength === 0) {
      return;
    }
    fs.writeSync(this.fileDescriptor, valueOrBuffer, safeOffset, safeLength);
  }

  public override flush(): void {
    if (this.fileDescriptor === null) {
      return;
    }
    fs.fsyncSync(this.fileDescriptor);
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
