export class Halton {
  public static Halton2(i: number): number {
    let value = BigInt(globalThis.Math.trunc(i));
    let h = value & 1n;
    let f = 2n;
    value >>= 1n;
    while (value !== 0n) {
      h <<= 1n;
      h += (value & 1n);
      value >>= 1n;
      f <<= 1n;
      h <<= 1n;
      h += (value & 1n);
      value >>= 1n;
      f <<= 1n;
      h <<= 1n;
      h += (value & 1n);
      value >>= 1n;
      f <<= 1n;
      h <<= 1n;
      h += (value & 1n);
      value >>= 1n;
      f <<= 1n;
    }

    return Number(h) / Number(f);
  }

  public static Halton3(i: number): number {
    let value = BigInt(globalThis.Math.trunc(i));
    let j = value;
    value /= 3n;
    let h = j - ((value << 1n) + value);
    let f = 3n;
    while (value > 0n) {
      const k = h - value;
      j = value;
      value /= 3n;
      h = j + (k << 1n) + k;
      f = (f << 1n) + f;
    }

    return Number(h) / Number(f);
  }

  public static Halton5(i: number): number {
    let value = BigInt(globalThis.Math.trunc(i));
    let j = value;
    value /= 5n;
    let h = j - ((value << 2n) + value);
    let f = 5n;
    while (value > 0n) {
      const k = h - value;
      j = value;
      value /= 5n;
      h = j + (k << 2n) + k;
      f = (f << 2n) + f;
    }

    return Number(h) / Number(f);
  }

  public static Halton7(i: number): number {
    let value = BigInt(globalThis.Math.trunc(i));
    let j = value;
    value /= 7n;
    let h = j - ((value << 2n) + (value << 1n) + value);
    let f = 7n;
    while (value > 0n) {
      const k = h - value;
      j = value;
      value /= 7n;
      h = j + (k << 2n) + (k << 1n) + k;
      f = (f << 2n) + (f << 1n) + f;
    }

    return Number(h) / Number(f);
  }

  private constructor() {
  }
}
