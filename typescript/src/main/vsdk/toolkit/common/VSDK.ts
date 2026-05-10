import { ColorRgb } from "./color/ColorRgb";
import { Vector2D } from "./linealAlgebra/Vector2D";
import { Vector3D } from "./linealAlgebra/Vector3D";

export class VSDK {
  public static readonly EPSILON = 1e-6;

  public static readonly WARNING = 1;
  public static readonly ERROR = 2;
  public static readonly FATAL_ERROR = 3;
  public static readonly DEBUG = 4;
  public static readonly VERBOSE = 5;
  private static withSystemExit = true;

  public static readonly POINT = 0;
  public static readonly LINE = 1;
  public static readonly TRIANGLE = 2;
  public static readonly TRIANGLE_STRIP = 3;
  public static readonly QUAD = 4;
  public static readonly QUAD_STRIP = 5;
  public static readonly PRIMITIVE_TYPE_COUNT = 6;

  public static readonly PLANE = 0;
  public static readonly SPHERE = 1;
  public static readonly CONE = 2;
  public static readonly INTERSECTION_TYPE_COUNT = 3;

  private static readonly primitiveCount = new Array<number>(VSDK.PRIMITIVE_TYPE_COUNT).fill(0);
  private static readonly intersectionCount = new Array<number>(VSDK.INTERSECTION_TYPE_COUNT).fill(0);

  public static resetPrimitiveCounters(): void {
    for (let i = 0; i < VSDK.PRIMITIVE_TYPE_COUNT; i++) {
      VSDK.primitiveCount[i] = 0;
    }
  }

  public static resetIntersectionCounters(): void {
    for (let i = 0; i < VSDK.INTERSECTION_TYPE_COUNT; i++) {
      VSDK.intersectionCount[i] = 0;
    }
  }

  public static acumulatePrimitiveCount(type: number, count: number): void {
    VSDK.primitiveCount[type] += count;
  }

  public static acumulateIntersectionCount(type: number, count: number): void {
    VSDK.intersectionCount[type] += count;
  }

  public static getPrimitiveCount(type: number): number {
    return VSDK.primitiveCount[type];
  }

  public static getIntersectionCount(type: number): number {
    return VSDK.intersectionCount[type];
  }

  public static equals(a: number, b: number): boolean {
    return globalThis.Math.abs(a - b) < VSDK.EPSILON;
  }

  public static vectorDistance(a: Vector3D, b: Vector3D): number;
  public static vectorDistance(a: Vector2D, b: Vector2D): number;
  public static vectorDistance(a: Vector3D | Vector2D, b: Vector3D | Vector2D): number {
    const ax = (a as Vector3D).x;
    const ay = (a as Vector3D).y;
    const bx = (b as Vector3D).x;
    const by = (b as Vector3D).y;
    const az = (a as Vector3D).z ?? 0.0;
    const bz = (b as Vector3D).z ?? 0.0;
    return globalThis.Math.sqrt(
      (ax - bx) * (ax - bx) +
      (ay - by) * (ay - by) +
      (az - bz) * (az - bz)
    );
  }

  public static colorDistance(a: ColorRgb, b: ColorRgb): number {
    return globalThis.Math.sqrt(
      (a.r - b.r) * (a.r - b.r) +
      (a.g - b.g) * (a.g - b.g) +
      (a.b - b.b) * (a.b - b.b)
    );
  }

  public static square(a: number): number {
    return a * a;
  }

  public static formatNumberWithinZeroes(a: number, n: number): string {
    return globalThis.Math.trunc(a).toString().padStart(n, "0");
  }

  public static formatDouble(a: number): string;
  public static formatDouble(a: number, digits: number): string;
  public static formatDouble(a: number, digits?: number): string {
    const safeDigits = digits === undefined ? 2 : digits;
    if (safeDigits <= 0) {
      return globalThis.Math.round(a).toString();
    }
    return a.toFixed(safeDigits);
  }

  public static formatByteAsHex(a: number): string {
    const i = VSDK.signedByte2unsignedInteger(a);
    const nibbleH = (i >> 4) & 0x0F;
    const nibbleL = i & 0x0F;
    const hex = "0123456789ABCDEF";
    return `${hex.charAt(nibbleH)}${hex.charAt(nibbleL)}`;
  }

  public static formatIntAsHex(a: number): string {
    let msg = "";
    msg += VSDK.formatByteAsHex((a & 0xFF000000) >> 24);
    msg += VSDK.formatByteAsHex((a & 0x00FF0000) >> 16);
    msg += VSDK.formatByteAsHex((a & 0x0000FF00) >> 8);
    msg += VSDK.formatByteAsHex(a & 0x000000FF);
    return msg;
  }

  public static signedByte2unsignedInteger(input: number): number {
    let a = input;
    if (a < 0) {
      a += 256;
    }
    return a;
  }

  public static unsigned8BitInteger2signedByte(input: number): number {
    let inValue = input;
    if (inValue > 255) {
      inValue = 255;
    }
    if (inValue < 0) {
      inValue = 0;
    }
    if (inValue > 127) {
      inValue -= 256;
    }
    return inValue;
  }

  private static classNameOf(o: unknown): string {
    if (o === null || o === undefined) {
      return "null";
    }
    const anyObject = o as { constructor?: { name?: string } };
    return anyObject.constructor?.name ?? typeof o;
  }

  public static reportMessageWithException(o: unknown, level: number, method: string, message: string, ee: unknown): void {
    let msg = "===========================================================================\n";
    msg += "= VSDK Exception report                                                   =\n";
    if (o !== null && o !== undefined) {
      msg += ` - An exception has been thrown in the "${VSDK.classNameOf(o)}" class\n`;
    }
    else {
      msg += " - An exception has been thrown from a static context\n";
    }
    msg += ` - Exception located at method ${method}\n`;
    msg += ` - Vitral exception message:\n${message}\n`;

    if (ee !== null && ee !== undefined) {
      const errorObject = ee as { name?: string; message?: string; stack?: string };
      msg += ` - Java exception class:\n${errorObject.name ?? "Error"}\n`;
      msg += ` - Java exception message:\n${errorObject.message ?? ""}\n`;
      if (errorObject.stack) {
        msg += `${errorObject.stack}\n`;
      }
    }
    else {
      msg += " - Java exception is null! No detailed information about error.\n";
    }
    msg += "===========================================================================\n";
    if (level === VSDK.FATAL_ERROR) {
      msg += "Program excecution suspended!\n";
    }

    process.stderr.write(msg);
    process.stderr.write("---------------------------------------------------------------------------\n");
    if (ee !== null && ee !== undefined) {
      const errorObject = ee as { message?: string; stack?: string };
      process.stderr.write(`${errorObject.message ?? ""}\n`);
      if (errorObject.stack) {
        process.stderr.write(`${errorObject.stack}\n`);
      }
    }
    else {
      process.stderr.write("Given exception is null! not reporting details!\n");
    }
    process.stderr.write("---------------------------------------------------------------------------\n");

    if (level === VSDK.FATAL_ERROR) {
      try {
        throw new globalThis.Error("VSDK.reportMessage(FATAL_ERROR)");
      }
      catch (e) {
        const report = e as globalThis.Error;
        process.stderr.write(`${report.message}\n`);
        if (report.stack) {
          process.stderr.write(`${report.stack}\n`);
        }
      }

      if (VSDK.withSystemExit) {
        process.exit(1);
      }
    }
  }

  public static reportMessage(o: unknown, level: number, method: string, message: string): void {
    let msg = "===========================================================================\n";
    msg += "= VSDK Exception report                                                   =\n";
    if (o !== null && o !== undefined) {
      msg += ` - An exception has been thrown in the "${VSDK.classNameOf(o)}" class\n`;
    }
    else {
      msg += " - An exception has been thrown from a static context\n";
    }
    msg += ` - Exception located at method ${method}\n`;
    msg += ` - Exception message:\n${message}\n`;
    msg += "===========================================================================\n";
    if (level === VSDK.FATAL_ERROR) {
      msg += "Program excecution suspended!\n";
    }

    process.stderr.write(msg);

    if (level === VSDK.FATAL_ERROR) {
      try {
        throw new globalThis.Error("VSDK.reportMessage(FATAL_ERROR)");
      }
      catch (e) {
        const report = e as globalThis.Error;
        process.stderr.write(`${report.message}\n`);
        if (report.stack) {
          process.stderr.write(`${report.stack}\n`);
        }
      }
      if (VSDK.withSystemExit) {
        process.exit(1);
      }
    }
  }

  public static setWithSystemExit(flag: boolean): void {
    VSDK.withSystemExit = flag;
  }

  public static formatSILengthUnit(x: number, digits: number): string {
    const postfixes = [
      "y", "_", "_", "z", "_", "_", "a", "_", "_", "f", "_", "_", "p", "_", "_", "n", "_", "_",
      "micro", "_", "_", "m", "c", "d", "", "_", "_", "K", "_", "_", "M", "_", "_", "G", "_", "_",
      "T", "_", "_", "P", "_", "_", "E", "_", "_", "Z", "_", "_", "Y"
    ];

    let corrected = x;
    let base = globalThis.Math.floor(globalThis.Math.log10(corrected));
    let multiplier = 1.0;

    while (base < -24) {
      multiplier /= 10.0;
      corrected *= 10.0;
      base = globalThis.Math.floor(globalThis.Math.log10(corrected));
    }
    while (base > 24) {
      multiplier *= 10.0;
      corrected /= 10.0;
      base = globalThis.Math.floor(globalThis.Math.log10(corrected));
    }

    let ibase = globalThis.Math.floor(base);
    while (postfixes[ibase + 24] === "_") {
      ibase--;
      multiplier *= 10.0;
    }

    base = globalThis.Math.floor(globalThis.Math.log10(corrected));
    const normalized = corrected / globalThis.Math.pow(10.0, base);
    return `${VSDK.formatDouble(multiplier * normalized, digits)} ${postfixes[ibase + 24]}m`;
  }
}
