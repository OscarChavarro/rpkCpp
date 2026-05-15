export class Float {
  public static readonly MIN_VALUE = 1.40129846e-45;
  public static readonly MAX_VALUE = 3.40282347e38;

  public static isFinite(a: number): boolean {
    return globalThis.Number.isFinite(a);
  }
}
