export class Math {
  public static readonly E = 2.7182818284590452354;
  public static readonly PI = 3.14159265358979323846;

  public static floor(a: number): number {
    return globalThis.Math.floor(a);
  }

  public static ceil(a: number): number {
    return globalThis.Math.ceil(a);
  }

  public static round(a: number): number {
    return a >= 0.0
      ? globalThis.Math.floor(a + 0.5)
      : globalThis.Math.ceil(a - 0.5);
  }

  public static log(a: number): number {
    return globalThis.Math.log(a);
  }

  public static log10(a: number): number {
    return globalThis.Math.log10(a);
  }

  public static sin(a: number): number {
    return globalThis.Math.sin(a);
  }

  public static cos(a: number): number {
    return globalThis.Math.cos(a);
  }

  public static tan(a: number): number {
    return globalThis.Math.tan(a);
  }

  public static acos(a: number): number {
    return globalThis.Math.acos(a);
  }

  public static atan(a: number): number {
    return globalThis.Math.atan(a);
  }

  public static exp(a: number): number {
    return globalThis.Math.exp(a);
  }

  public static pow(a: number, e: number): number {
    return globalThis.Math.pow(a, e);
  }

  public static abs(a: number): number {
    return globalThis.Math.abs(a);
  }

  public static min(a: number, b: number): number {
    return a < b ? a : b;
  }

  public static max(a: number, b: number): number {
    return a > b ? a : b;
  }

  public static sqrt(a: number): number {
    return globalThis.Math.sqrt(a);
  }

  public static getExponent(a: number): number {
    if (a === 0 || !globalThis.Number.isFinite(a)) {
      return 0;
    }
    return globalThis.Math.floor(globalThis.Math.log2(globalThis.Math.abs(a)));
  }

  public static scalb(a: number, scaleFactor: number): number {
    return a * globalThis.Math.pow(2.0, scaleFactor);
  }
}
