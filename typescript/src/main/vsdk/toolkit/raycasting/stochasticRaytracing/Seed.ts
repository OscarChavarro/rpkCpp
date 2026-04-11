/**
Configuration class
*/
export class Seed {
  private readonly m_seed: number[];

  public constructor() {
    this.m_seed = [0, 0, 0];
  }

  public GetSeed(): number[] {
    return this.m_seed;
  }

  public SetSeed(seed: Seed): void;
  public SetSeed(seed16v: number[]): void;
  public SetSeed(s0: number, s1: number, s2: number): void;
  public SetSeed(a: Seed | number[] | number, b?: number, c?: number): void {
    if (a instanceof Seed) {
      const s = a.GetSeed();
      this.m_seed[0] = s[0] | 0;
      this.m_seed[1] = s[1] | 0;
      this.m_seed[2] = s[2] | 0;
      return;
    }

    if (Array.isArray(a)) {
      this.m_seed[0] = (a[0] ?? 0) | 0;
      this.m_seed[1] = (a[1] ?? 0) | 0;
      this.m_seed[2] = (a[2] ?? 0) | 0;
      return;
    }

    this.m_seed[0] = a | 0;
    this.m_seed[1] = (b ?? 0) | 0;
    this.m_seed[2] = (c ?? 0) | 0;
  }

  public XORSeed(xOrSeed: Seed): void {
    const s = xOrSeed.GetSeed();
    this.m_seed[0] ^= s[0];
    this.m_seed[1] ^= s[1];
    this.m_seed[2] ^= s[2];
  }
}
