import { Niederreiter63 } from "./Niederreiter63";

export class Niederreiter {
  public static readonly DIMEN = Niederreiter63.DIMEN;
  public static readonly NBITS = Niederreiter63.NBITS;
  public static readonly SKIP = Niederreiter63.SKIP;
  public static readonly NBITS_POW = Niederreiter63.NBITS_POW;
  public static readonly NBITS_POW1 = Niederreiter63.NBITS_POW1;
  public static readonly RECIP = Niederreiter63.RECIP;
  public static readonly RECIP1 = Niederreiter63.RECIP1;

  public static Nied(n: bigint | number): bigint[] {
    return Niederreiter63.Nied63(n);
  }

  public static NextNiedInRange(
    idx: bigint[],
    dir: number,
    nmsb: number,
    msb1: bigint,
    rmsb2: bigint
  ): bigint[] | null {
    return Niederreiter63.NextNiedInRange63(idx, dir, nmsb, msb1, rmsb2);
  }

  public static radicalInverse(n: bigint | number): bigint {
    return Niederreiter63.radicalInverse63(n);
  }

  public static foldSample(xi1: bigint[], xi2: bigint[]): void {
    Niederreiter63.foldSample63(xi1, xi2);
  }

  private constructor() {
  }
}
