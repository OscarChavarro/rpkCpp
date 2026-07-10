import { Random48 } from "../../common/Random48";
import { Seed } from "./Seed";

export class SeedConfig {
  private m_seeds: Seed[] | null;
  private static readonly xOrSeed = new Seed();

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

  /**
  Saves the current seed and generates a new seed based on the current
  seed, mirroring the C++ port's use of the libc seed48() interface.
  */
  public save(depth: number): void {
    if (this.m_seeds === null || depth < 0 || depth >= this.m_seeds.length) {
      return;
    }

    // Save the seed (supply dummy seed to seed48())
    const current = this.m_seeds[depth]!;
    current.SetSeed(Random48.seed48(current.GetSeed()));

    // Generate a new seed, dependent on the current seed
    const tmpSeed = new Seed();

    tmpSeed.SetSeed(current);
    // Fixed xor should do the trick. Note that you can not use
    // the random number generator itself to generate new seeds,
    // because the supplied random numbers *are* the (truncated) seeds
    tmpSeed.XORSeed(SeedConfig.xOrSeed);
    // Set the new seed and drand48 once, to be sure
    Random48.seed48(tmpSeed.GetSeed());
    Random48.drand48();
  }

  /**
  Restores seed for a certain depth.
  */
  public Restore(depth: number): void {
    if (this.m_seeds === null || depth < 0 || depth >= this.m_seeds.length) {
      return;
    }
    Random48.seed48(this.m_seeds[depth]!.GetSeed());
  }
}
