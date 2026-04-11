export class Random {
  private static readonly MULTIPLIER = 0x5DEECE66Dn;
  private static readonly ADDEND = 0xBn;
  private static readonly MASK = (1n << 48n) - 1n;

  private seed: bigint;

  public constructor(seed: number = Date.now()) {
    this.seed = 0n;
    this.setSeed(seed);
  }

  public setSeed(seed: number): void {
    this.seed = (BigInt(globalThis.Math.trunc(seed)) ^ Random.MULTIPLIER) & Random.MASK;
  }

  protected next(bits: number): number {
    this.seed = (this.seed * Random.MULTIPLIER + Random.ADDEND) & Random.MASK;
    return Number(this.seed >> BigInt(48 - bits));
  }

  public nextDouble(): number {
    const high = BigInt(this.next(26));
    const low = BigInt(this.next(27));
    const combined = (high << 27n) + low;
    return Number(combined) / Number(1n << 53n);
  }
}
