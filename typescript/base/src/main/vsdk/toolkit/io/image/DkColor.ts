import { OutputStream } from "../../../../java/io/OutputStream";
import { ColorRgb } from "../../common/color/ColorRgb";
import { CppReAlloc } from "../../common/memoryManagement/CppReAlloc";

export class DkColor {
  private static readonly RED = 0;
  private static readonly GREEN = 1;
  private static readonly BLUE = 2;
  private static readonly EXP = 3;
  private static readonly COL_XS = 128;
  private static readonly MINIMUM_SCAN_LINE_LENGTH = 8;
  private static readonly MAXIMUM_SCAN_LINE_LENGTH = 0x7fff;
  private static readonly MINIMUM_RUN_LENGTH = 4;

  private static temporaryBuffer: Uint8Array | null = null;
  private static temporaryBufferLength = 0;

  private constructor() {
  }

  private static writeByte(stream: OutputStream | null, value: number): void {
    if (stream === null) {
      return;
    }
    try {
      stream.write(value & 0xFF);
    }
    catch (_ignored) {
    }
  }

  private static writeBytes(stream: OutputStream | null, bytes: Uint8Array, length: number): void {
    if (stream === null || length <= 0) {
      return;
    }
    try {
      stream.write(bytes, 0, length);
    }
    catch (_ignored) {
    }
  }

  private static javaIntCast(value: number): number {
    if (Number.isNaN(value)) {
      return 0;
    }
    if (value >= 2147483647.0) {
      return 2147483647;
    }
    if (value <= -2147483648.0) {
      return -2147483648;
    }
    return value < 0.0 ? globalThis.Math.ceil(value) : globalThis.Math.floor(value);
  }

  private static javaByte(value: number): number {
    return value & 0xFF;
  }

  private static getExponent(value: number): number {
    if (Number.isNaN(value) || value === 0.0) {
      return -1023;
    }
    if (!Number.isFinite(value)) {
      return 1024;
    }
    return globalThis.Math.floor(globalThis.Math.log2(globalThis.Math.abs(value)));
  }

  private static scalb(value: number, scaleFactor: number): number {
    return value * globalThis.Math.pow(2.0, scaleFactor);
  }

  private static tempBuffer(length: number): Uint8Array | null {
    if (length > DkColor.temporaryBufferLength) {
      if (DkColor.temporaryBufferLength > 0) {
        DkColor.temporaryBuffer = CppReAlloc.reAlloc(
          DkColor.temporaryBuffer,
          DkColor.temporaryBufferLength,
          length
        ) as Uint8Array | null;
      }
      else {
        DkColor.temporaryBuffer = new Uint8Array(length);
      }
      DkColor.temporaryBufferLength = DkColor.temporaryBuffer === null ? 0 : length;
    }
    return DkColor.temporaryBuffer;
  }

  private static writeByteColors(scanline: Uint8Array, len: number, outputStream: OutputStream | null): number {
    let cnt = 0;
    let c2: number;

    if (outputStream === null) {
      return -1;
    }

    if (len < DkColor.MINIMUM_SCAN_LINE_LENGTH || len > DkColor.MAXIMUM_SCAN_LINE_LENGTH) {
      DkColor.writeBytes(outputStream, scanline, len * 4);
      return 0;
    }

    DkColor.writeByte(outputStream, 2);
    DkColor.writeByte(outputStream, 2);
    DkColor.writeByte(outputStream, len >> 8);
    DkColor.writeByte(outputStream, len & 255);

    for (let i = 0; i < 4; i++) {
      for (let j = 0; j < len; j += cnt) {
        let beg: number;

        for (beg = j; beg < len; beg += cnt) {
          for (
            cnt = 1;
            cnt < 127 &&
            beg + cnt < len &&
            scanline[(beg + cnt) * 4 + i] === scanline[beg * 4 + i];
            cnt++
          ) {
          }
          if (cnt >= DkColor.MINIMUM_RUN_LENGTH) {
            break;
          }
        }

        if (beg - j > 1 && beg - j < DkColor.MINIMUM_RUN_LENGTH) {
          c2 = j + 1;
          while (c2 < beg && scanline[c2 * 4 + i] === scanline[j * 4 + i]) {
            c2++;
            if (c2 === beg) {
              DkColor.writeByte(outputStream, 128 + beg - j);
              DkColor.writeByte(outputStream, scanline[j * 4 + i]);
              j = beg;
              break;
            }
          }
        }

        while (j < beg) {
          c2 = beg - j;
          if (c2 > 128) {
            c2 = 128;
          }
          DkColor.writeByte(outputStream, c2);
          while (c2-- > 0) {
            DkColor.writeByte(outputStream, scanline[j * 4 + i]);
            j++;
          }
        }

        if (cnt >= DkColor.MINIMUM_RUN_LENGTH) {
          DkColor.writeByte(outputStream, 128 + cnt);
          DkColor.writeByte(outputStream, scanline[beg * 4 + i]);
        }
        else {
          cnt = 0;
        }
      }
    }

    return 0;
  }

  private static setByteColors(scanline: Uint8Array, base: number, r: number, g: number, b: number): void {
    let d = r > g ? r : g;
    if (b > d) {
      d = b;
    }

    if (d < 0) {
      scanline[base + DkColor.RED] = 0;
      scanline[base + DkColor.GREEN] = 0;
      scanline[base + DkColor.BLUE] = 0;
      scanline[base + DkColor.EXP] = 0;
      return;
    }

    const e = DkColor.getExponent(d) + 1;
    const normalized = DkColor.scalb(d, -e);
    d = normalized * 255.9999 / d;

    scanline[base + DkColor.RED] = DkColor.javaByte(DkColor.javaIntCast(r * d));
    scanline[base + DkColor.GREEN] = DkColor.javaByte(DkColor.javaIntCast(g * d));
    scanline[base + DkColor.BLUE] = DkColor.javaByte(DkColor.javaIntCast(b * d));
    scanline[base + DkColor.EXP] = DkColor.javaByte(DkColor.javaIntCast(e + DkColor.COL_XS));
  }

  public static writeScan(scanline: number[][], len: number, outputStream: OutputStream | null): number;
  public static writeScan(scanline: ColorRgb[], len: number, outputStream: OutputStream | null): number;
  public static writeScan(
    scanline: number[][] | ColorRgb[],
    len: number,
    outputStream: OutputStream | null
  ): number {
    const colorScan = DkColor.tempBuffer(len * 4);
    if (colorScan === null) {
      return -1;
    }

    if (scanline.length > 0 && scanline[0] instanceof ColorRgb) {
      const colorArray = scanline as ColorRgb[];
      for (let n = 0; n < len; n++) {
        const base = n * 4;
        DkColor.setByteColors(colorScan, base, colorArray[n].r, colorArray[n].g, colorArray[n].b);
      }
    }
    else {
      const floatArray = scanline as number[][];
      for (let n = 0; n < len; n++) {
        const base = n * 4;
        DkColor.setByteColors(
          colorScan,
          base,
          floatArray[n][DkColor.RED],
          floatArray[n][DkColor.GREEN],
          floatArray[n][DkColor.BLUE]
        );
      }
    }

    return DkColor.writeByteColors(colorScan, len, outputStream);
  }

  public static freeBuffer(): void {
    if (DkColor.temporaryBuffer !== null) {
      DkColor.temporaryBuffer = null;
      DkColor.temporaryBufferLength = 0;
    }
  }
}
