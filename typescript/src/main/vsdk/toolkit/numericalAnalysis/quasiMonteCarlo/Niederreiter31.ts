import { NiederreiterCore } from "./NiederreiterCore";

export class Niederreiter31 {
  public static readonly SKIP = 4096;
  public static readonly DIMEN = 4;
  public static readonly NBITS = 31;
  public static readonly RECIP = 1.0 / 2147483648.0;
  public static readonly RECIP1 = 2147483648.0;
  public static readonly NBITS_POW = 2147483648;
  public static readonly NBITS_POW1 = 1073741824;

  private static readonly directionNumbers: bigint[][] = [
    [
      0x40000000n, 0x20000000n, 0x10000000n, 0x08000000n,
      0x04000000n, 0x02000000n, 0x01000000n, 0x00800000n,
      0x00400000n, 0x00200000n, 0x00100000n, 0x00080000n,
      0x00040000n, 0x00020000n, 0x00010000n, 0x00008000n,
      0x00004000n, 0x00002000n, 0x00001000n, 0x00000800n,
      0x00000400n, 0x00000200n, 0x00000100n, 0x00000080n,
      0x00000040n, 0x00000020n, 0x00000010n, 0x00000008n,
      0x00000004n, 0x00000002n, 0x00000001n
    ],
    [
      0x40000000n, 0x60000000n, 0x50000000n, 0x78000000n,
      0x44000000n, 0x66000000n, 0x55000000n, 0x7f800000n,
      0x40400000n, 0x60600000n, 0x50500000n, 0x78780000n,
      0x44440000n, 0x66660000n, 0x55550000n, 0x7fff8000n,
      0x40004000n, 0x60006000n, 0x50005000n, 0x78007800n,
      0x44004400n, 0x66006600n, 0x55005500n, 0x7f807f80n,
      0x40404040n, 0x60606060n, 0x50505050n, 0x78787878n,
      0x44444444n, 0x66666666n, 0x55555555n
    ],
    [
      0x60000000n, 0x48000000n, 0x38000000n, 0x7a000000n,
      0x5e000000n, 0x36800000n, 0x65800000n, 0x4b200000n,
      0x3e600000n, 0x7ec80000n, 0x5db80000n, 0x315a0000n,
      0x603e0000n, 0x487e8000n, 0x385d8000n, 0x7a312000n,
      0x5e606000n, 0x36c84800n, 0x65b83800n, 0x4b5a7a00n,
      0x3e3e5e00n, 0x7efeb680n, 0x5ddde580n, 0x31116b20n,
      0x60005e60n, 0x480036c8n, 0x380065b8n, 0x7a004b5an,
      0x5e003e3en, 0x36807efen, 0x65805dddn
    ],
    [
      0x70000000n, 0x62000000n, 0x46000000n, 0x1e000000n,
      0x2c400000n, 0x5ac00000n, 0x37c00000n, 0x7d880000n,
      0x6b580000n, 0x44b80000n, 0x1b310000n, 0x26230000n,
      0x5c470000n, 0x38c62000n, 0x71c46000n, 0x6389e000n,
      0x471ac400n, 0x1e7dac00n, 0x2cf27c00n, 0x5bacd880n,
      0x3719b580n, 0x7c7a6b80n, 0x6af4d310n, 0x45a88230n,
      0x1b580070n, 0x26b80062n, 0x5d310046n, 0x3823001en,
      0x7007002cn, 0x6206205an, 0x46046037n
    ]
  ];

  private static readonly core = new NiederreiterCore(
    Niederreiter31.directionNumbers,
    BigInt(Niederreiter31.SKIP),
    1n << BigInt(Niederreiter31.NBITS),
    1n << BigInt(Niederreiter31.NBITS - 1),
    Niederreiter31.DIMEN,
    Niederreiter31.NBITS
  );

  private static mapSample(sample: bigint[]): number[] {
    const mapped = new Array<number>(Niederreiter31.DIMEN);
    for (let i = 0; i < Niederreiter31.DIMEN; i++) {
      mapped[i] = Number(sample[i]);
    }
    return mapped;
  }

  public static niederreiter31(index: number): number[] {
    return Niederreiter31.mapSample(Niederreiter31.core.sample(BigInt(globalThis.Math.trunc(index))));
  }

  public static NextNiedInRange31(
    idx: number[],
    dir: number,
    nmsb: number,
    msb1: number,
    rmsb2: number
  ): number[] | null {
    const idxBig = [BigInt(globalThis.Math.trunc(idx[0]))];
    const sample = Niederreiter31.core.nextInRange(
      idxBig,
      dir,
      nmsb,
      BigInt(globalThis.Math.trunc(msb1)),
      BigInt(globalThis.Math.trunc(rmsb2))
    );
    if (sample === null) {
      return null;
    }
    idx[0] = Number(idxBig[0]);
    return Niederreiter31.mapSample(sample);
  }

  public static radicalInverse31(n: number): number {
    return Number(Niederreiter31.core.radicalInverse(BigInt(globalThis.Math.trunc(n))));
  }

  public static foldSample31(xi1: number[], xi2: number[]): void {
    const x1 = [BigInt(globalThis.Math.trunc(xi1[0]))];
    const x2 = [BigInt(globalThis.Math.trunc(xi2[0]))];
    Niederreiter31.core.foldSample(x1, x2);
    xi1[0] = Number(x1[0]);
    xi2[0] = Number(x2[0]);
  }

  private constructor() {
  }
}
