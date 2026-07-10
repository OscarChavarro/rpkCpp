/**
Central 48-bit RNG used by the stochastic raytracing code, replicating the
POSIX rand48 family (drand48/srand48/seed48) exactly as glibc implements it,
so that this port draws the very same random sequence as the C++ port and
produces bit-identical golden test images.

Note that glibc leaves the 48-bit state zero-initialized until srand48 or
seed48 is called (it does not apply the POSIX default seed 0x1234ABCD330E),
and the C++ port renders without seeding, so the state starts at zero here
as well.
*/
export class Random48 {
  // Same constants glibc uses for drand48/srand48/seed48
  private static readonly MULTIPLIER = 0x5DEECE66Dn;
  private static readonly ADDEND = 0xBn;
  private static readonly MASK_48 = (1n << 48n) - 1n;
  private static readonly TWO_POW_48 = 281474976710656.0; // 2^48

  private static state = 0n;

  private constructor() {
  }

  public static srand48(seed: number): void {
    const high = BigInt(globalThis.Math.trunc(seed)) & 0xFFFFFFFFn;
    Random48.state = ((high << 16n) | 0x330En) & Random48.MASK_48;
  }

  public static drand48(): number {
    Random48.state = (Random48.MULTIPLIER * Random48.state + Random48.ADDEND) & Random48.MASK_48;
    return Number(Random48.state) / Random48.TWO_POW_48;
  }

  /**
  Sets the 48-bit generator state from newSeed[3] (each entry treated as an
  unsigned 16-bit value, low word first) and returns the previous state,
  mirroring the C library's seed48() semantics used by SeedConfig.
  */
  public static seed48(newSeed: number[]): number[] {
    const previous = [
      Number(Random48.state & 0xFFFFn),
      Number((Random48.state >> 16n) & 0xFFFFn),
      Number((Random48.state >> 32n) & 0xFFFFn)
    ];
    Random48.state = (
      (BigInt((newSeed[2] ?? 0) & 0xFFFF) << 32n)
      | (BigInt((newSeed[1] ?? 0) & 0xFFFF) << 16n)
      | BigInt((newSeed[0] ?? 0) & 0xFFFF)
    ) & Random48.MASK_48;
    return previous;
  }
}
