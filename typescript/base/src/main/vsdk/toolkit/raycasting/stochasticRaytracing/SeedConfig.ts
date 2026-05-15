import { Seed } from "./Seed";

export class SeedConfig {
  private m_seeds: Seed[] | null;
  private static readonly xOrSeed = new Seed();

  private static randomFromSeed(seed: Seed): () => number {
    const s = seed.GetSeed();
    let state = (
      ((BigInt(s[0] & 0xFFFF) << 32n)
      ^ (BigInt(s[1] & 0xFFFF) << 16n)
      ^ BigInt(s[2] & 0xFFFF))
      & 0xFFFFFFFFFFFFFFFFn
    );

    return (): number => {
      state = (state * 6364136223846793005n + 1442695040888963407n) & 0xFFFFFFFFFFFFFFFFn;
      const x = Number((state >> 11n) & ((1n << 53n) - 1n));
      return x / 9007199254740992.0;
    };
  }

  public constructor() {
    SeedConfig.xOrSeed.SetSeed(0xF0, 0x65, 0xDE);
    this.m_seeds = null;
  }

  public clear(): void {
    this.m_seeds = null;
  }

  public init(maxDepth: number): void {
    this.clear();
    this.m_seeds = new Array<Seed>(maxDepth);
    for (let i = 0; i < maxDepth; i++) {
      this.m_seeds[i] = new Seed();
    }
  }

  public save(depth: number): void {
    if (this.m_seeds === null || depth < 0 || depth >= this.m_seeds.length) {
      return;
    }

    const current = this.m_seeds[depth];
    const tmpSeed = new Seed();

    tmpSeed.SetSeed(current);
    tmpSeed.XORSeed(SeedConfig.xOrSeed);

    const random = SeedConfig.randomFromSeed(tmpSeed);
    const s0 = globalThis.Math.trunc(random() * 0x10000) & 0xFFFF;
    const s1 = globalThis.Math.trunc(random() * 0x10000) & 0xFFFF;
    const s2 = globalThis.Math.trunc(random() * 0x10000) & 0xFFFF;
    current.SetSeed([s0, s1, s2]);

    random();
  }

  public Restore(depth: number): void {
    void depth;
    // Java port keeps seeds only; explicit global RNG restore is not required.
  }
}
