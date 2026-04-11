import { ColorRgb } from "../common/ColorRgb";

export class Texture {
  private readonly width: number;
  private readonly height: number;
  private readonly channels: number;
  private readonly data: Uint8Array | null;

  private static setMonochrome(rgb: ColorRgb, val: number): void {
    rgb.set(val, val, val);
  }

  public constructor();
  public constructor(inWidth: number, inHeight: number, inChannels: number, inData: Uint8Array | null);
  public constructor(inWidth?: number, inHeight?: number, inChannels?: number, inData?: Uint8Array | null) {
    if (
      inWidth === undefined || inHeight === undefined || inChannels === undefined || inData === undefined
    ) {
      this.width = 0;
      this.height = 0;
      this.channels = 0;
      this.data = null;
      return;
    }

    this.width = inWidth;
    this.height = inHeight;
    this.channels = inChannels;

    const byteCount = this.width * this.height * this.channels;
    if (byteCount <= 0 || inData === null) {
      this.data = null;
      return;
    }
    this.data = new Uint8Array(inData.slice(0, byteCount));
  }

  public getWidth(): number {
    return this.width;
  }

  public getHeight(): number {
    return this.height;
  }

  public getChannels(): number {
    return this.channels;
  }

  public getData(): Uint8Array | null {
    return this.data;
  }

  public evaluateColor(u: number, v: number): ColorRgb {
    const rgb = new ColorRgb();
    rgb.clear();

    if (this.data === null || this.width <= 0 || this.height <= 0 || this.channels <= 0) {
      return rgb;
    }

    const u1 = u - globalThis.Math.floor(u);
    const u0 = 1.0 - u1;
    const v1 = v - globalThis.Math.floor(v);
    const v0 = 1.0 - v1;

    let i = globalThis.Math.trunc(u1 * this.width);
    let i1 = i + 1;
    let j = globalThis.Math.trunc(v1 * this.height);
    let j1 = j + 1;

    if (i < 0) {
      i = 0;
    }
    if (i >= this.width) {
      i = this.width - 1;
    }
    if (j < 0) {
      j = 0;
    }
    if (j >= this.height) {
      j = this.height - 1;
    }
    if (i1 >= this.width) {
      i1 -= this.width;
    }
    if (j1 >= this.height) {
      j1 -= this.height;
    }

    const pixelIndex00 = (j * this.width + i) * this.channels;
    const pixelIndex01 = (j1 * this.width + i) * this.channels;
    const pixelIndex10 = (j * this.width + i1) * this.channels;
    const pixelIndex11 = (j1 * this.width + i1) * this.channels;

    const rgb00 = new ColorRgb();
    const rgb10 = new ColorRgb();
    const rgb01 = new ColorRgb();
    const rgb11 = new ColorRgb();

    switch (this.channels) {
      case 1:
        Texture.setMonochrome(rgb00, this.channelValue(pixelIndex00, 0));
        Texture.setMonochrome(rgb10, this.channelValue(pixelIndex10, 0));
        Texture.setMonochrome(rgb01, this.channelValue(pixelIndex01, 0));
        Texture.setMonochrome(rgb11, this.channelValue(pixelIndex11, 0));
        break;
      case 3:
      case 4:
        rgb00.set(this.channelValue(pixelIndex00, 0), this.channelValue(pixelIndex00, 1), this.channelValue(pixelIndex00, 2));
        rgb10.set(this.channelValue(pixelIndex10, 0), this.channelValue(pixelIndex10, 1), this.channelValue(pixelIndex10, 2));
        rgb01.set(this.channelValue(pixelIndex01, 0), this.channelValue(pixelIndex01, 1), this.channelValue(pixelIndex01, 2));
        rgb11.set(this.channelValue(pixelIndex11, 0), this.channelValue(pixelIndex11, 1), this.channelValue(pixelIndex11, 2));
        break;
      default:
        break;
    }

    rgb.set(
      0.25 * (u0 * v0 * rgb00.r + u1 * v0 * rgb10.r + u0 * v1 * rgb01.r + u1 * v1 * rgb11.r),
      0.25 * (u0 * v0 * rgb00.g + u1 * v0 * rgb10.g + u0 * v1 * rgb01.g + u1 * v1 * rgb11.g),
      0.25 * (u0 * v0 * rgb00.b + u1 * v0 * rgb10.b + u0 * v1 * rgb01.b + u1 * v1 * rgb11.b)
    );

    return rgb;
  }

  private channelValue(pixelIndex: number, channel: number): number {
    if (this.data === null) {
      return 0.0;
    }
    const value = this.data[pixelIndex + channel] & 0xFF;
    return value / 255.0;
  }
}
