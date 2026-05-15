import { OutputStream } from "../../../../java/io/OutputStream";
import { ImageOutputHandle } from "./ImageOutputHandle";

export class PPMOutputHandle extends ImageOutputHandle {
  private outputStream: OutputStream | null;

  public constructor(output: OutputStream, w: number, h: number) {
    super();
    this.init("PPM", w, h);
    this.outputStream = output;

    if (this.outputStream !== null) {
      const header = `P6\n${this.width} ${this.height}\n255\n`;
      const headerBytes = new Uint8Array(Buffer.from(header, "ascii"));
      if (headerBytes.length > 0) {
        this.writeBytes(headerBytes, headerBytes.length);
      }
    }
  }

  private writeBytes(bytes: Uint8Array, length: number): void {
    if (this.outputStream === null || length <= 0) {
      return;
    }
    try {
      this.outputStream.write(bytes, 0, length);
    }
    catch (_ignored) {
    }
  }

  public override writeDisplayRGB(x: Uint8Array): number;
  public override writeDisplayRGB(x: number[]): number;
  public override writeDisplayRGB(x: Uint8Array | number[]): number {
    if (!(x instanceof Uint8Array)) {
      return super.writeDisplayRGB(x);
    }

    if (this.outputStream !== null) {
      this.writeBytes(x, 3 * this.width);
      return this.width;
    }
    return 0;
  }
}
