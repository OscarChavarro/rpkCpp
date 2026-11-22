export class NiederreiterCore {
  private readonly cj: bigint[][];
  private readonly skip: bigint;
  private readonly nBitsPow: bigint;
  private readonly nBitsPow1: bigint;
  private readonly dimension: number;
  private readonly numberOfBits: number;
  private readonly nied: bigint[];
  private count: bigint;

  public constructor(
    cj: bigint[][],
    skip: bigint,
    nBitsPow: bigint,
    nBitsPow1: bigint,
    dimension: number,
    numberOfBits: number
  ) {
    this.cj = cj;
    this.skip = skip;
    this.nBitsPow = nBitsPow;
    this.nBitsPow1 = nBitsPow1;
    this.dimension = dimension;
    this.numberOfBits = numberOfBits;
    this.nied = new Array<bigint>(dimension).fill(0n);
    this.count = 0n;
  }

  private mask(value: bigint): bigint {
    const wordMask = (1n << BigInt(this.numberOfBits + 1)) - 1n;
    return value & wordMask;
  }

  private compareUnsigned(a: bigint, b: bigint): number {
    const ua = this.mask(a);
    const ub = this.mask(b);
    if (ua < ub) {
      return -1;
    }
    if (ua > ub) {
      return 1;
    }
    return 0;
  }

  public sample(n: bigint): bigint[] {
    n = this.mask(n + this.skip);
    let diff = this.mask(n ^ this.count);
    let bitIndex = 0;

    while (diff !== 0n) {
      if ((diff & 1n) !== 0n) {
        this.nied[0]! ^= this.cj[0]![bitIndex]!;
        this.nied[1]! ^= this.cj[1]![bitIndex]!;
        this.nied[2]! ^= this.cj[2]![bitIndex]!;
        this.nied[3]! ^= this.cj[3]![bitIndex]!;
      }
      bitIndex++;
      diff >>= 1n;
    }

    this.count = n;
    return this.nied;
  }

  public nextInRange(
    idx: bigint[],
    dir: number,
    nmsb: number,
    msb1: bigint,
    rmsb2: bigint
  ): bigint[] | null {
    let step = 1n << BigInt(nmsb);
    const mask = step - 1n;
    const rmask = mask << BigInt(this.numberOfBits - nmsb);
    msb1 &= mask;
    rmsb2 &= rmask;

    let i = this.mask(idx[0]! + this.skip);
    if (dir >= 0) {
      const condition = this.compareUnsigned(i & mask, msb1) <= 0;
      i = this.mask(((condition ? i : this.mask(i + mask)) & this.mask(~mask)) | msb1);
    }
    else {
      const condition = this.compareUnsigned(i & mask, msb1) >= 0;
      i = this.mask(((condition ? i : this.mask(i - mask)) & this.mask(~mask)) | msb1);
      step = -step;
    }

    let c = this.count;
    let diff = this.mask((i ^ c) & mask);
    let bitIndex = 0;
    while (diff !== 0n) {
      if ((diff & 1n) !== 0n) {
        this.nied[1]! ^= this.cj[1]![bitIndex]!;
      }
      diff >>= 1n;
      bitIndex++;
    }

    do {
      diff = (i ^ c) >> BigInt(nmsb);
      bitIndex = nmsb;
      while (diff !== 0n) {
        if ((diff & 1n) !== 0n) {
          this.nied[1]! ^= this.cj[1]![bitIndex]!;
        }
        diff >>= 1n;
        bitIndex++;
      }
      c = i;
      i = this.mask(i + step);
      if (this.compareUnsigned(i, this.nBitsPow) >= 0) {
        process.stderr.write(
          `\nOverflow in Niederreiter sequence. A ${this.numberOfBits}-bit sequence is not enough???\n`
        );
        return null;
      }
    }
    while ((this.nied[1]! & rmask) !== rmsb2);

    diff = this.mask(c ^ this.count);
    bitIndex = 0;
    while (diff !== 0n) {
      if ((diff & 1n) !== 0n) {
        this.nied[0]! ^= this.cj[0]![bitIndex]!;
        this.nied[2]! ^= this.cj[2]![bitIndex]!;
        this.nied[3]! ^= this.cj[3]![bitIndex]!;
      }
      diff >>= 1n;
      bitIndex++;
    }
    this.count = c;

    idx[0] = this.mask(this.count - this.skip);
    return this.nied;
  }

  public radicalInverse(n: bigint): bigint {
    n = this.mask(n);
    let inv = 0n;
    let f = this.nBitsPow1;

    while (n !== 0n) {
      if ((n & 1n) !== 0n) {
        inv |= f;
      }
      f >>= 1n;
      n >>= 1n;
      if ((n & 1n) !== 0n) {
        inv |= f;
      }
      f >>= 1n;
      n >>= 1n;
      if ((n & 1n) !== 0n) {
        inv |= f;
      }
      f >>= 1n;
      n >>= 1n;
      if ((n & 1n) !== 0n) {
        inv |= f;
      }
      f >>= 1n;
      n >>= 1n;
    }

    return inv;
  }

  public foldSample(xi1: bigint[], xi2: bigint[]): void {
    let u = this.mask(xi1[0]!);
    let v = this.mask(xi2[0]!);

    u = (u & this.mask(~3n)) | 1n;
    v = (v & this.mask(~3n)) | 1n;
    let d = (u & v) & this.mask(~1n);

    let m = this.nBitsPow;
    while (d !== 0n) {
      if ((d & this.nBitsPow1) !== 0n) {
        u = this.mask((u & m) | (this.mask(~this.mask(u - 1n)) & this.mask(~m)));
        v = this.mask((v & m) | (this.mask(~this.mask(v - 1n)) & this.mask(~m)));
      }
      m = this.mask(m | (m >> 1n));
      d = this.mask(d << 1n);
    }

    xi1[0] = u;
    xi2[0] = v;
  }
}
