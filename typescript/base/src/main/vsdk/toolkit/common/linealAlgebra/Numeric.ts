export class Numeric {
  public static readonly HUGE_DOUBLE_VALUE = 1e30;
  public static readonly HUGE_FLOAT_VALUE = 3.40282347e38;
  public static readonly EPSILON = 1e-6;
  public static readonly EPSILON_FLOAT = 1e-6;

  private constructor() {
  }

  public static doubleEqual(a: number, b: number, tolerance: number): boolean {
    return (a - b) > -tolerance && (a - b) < tolerance;
  }

  public static floatCompare(x: number, y: number): boolean {
    return x > y;
  }

  public static roundDeltaToZero(x: number[] | null, epsilon: number): void {
    if (x === null || x.length === 0) {
      return;
    }
    if (x[0] <= epsilon && x[0] >= -epsilon) {
      x[0] = 0.0;
    }
  }
}
