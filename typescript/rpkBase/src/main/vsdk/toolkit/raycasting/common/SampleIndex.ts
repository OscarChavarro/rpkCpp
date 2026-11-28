/**
The 64 bit sample index of a pixel traced in parallel (C++'s uint64_t
*sampleIndex of StochasticRaytracingConfiguration and StratifiedSampling2D):
each pixel has its own range of indices, so the scheduling of the workers does
not change the samples it receives.

TypeScript adaptation: the index is held as two unsigned 32 bit halves, and
the SplitMix64 finalizer of C++'s sampleUnitInterval is computed on them with
exact 32 bit arithmetic, giving the same bits as the C++ uint64_t code.
*/
export class SampleIndex {
  private high = 0;
  private low = 0;

  /** C++: sampleIndex = (pixel ^ static_cast<uint32_t>(baseSeed)) << 32 */
  public startPixel(pixel: number, baseSeed: number): void {
    // pixel = y * width + x is below 2^32, so its upper half is zero
    this.high = ((pixel >>> 0) ^ (baseSeed >>> 0)) >>> 0;
    this.low = 0;
  }

  /**
  C++ sampleUnitInterval: SplitMix64 finalizer of the current index (which is
  then incremented), as a double in [0, 1)
  */
  public nextUnitInterval(): number {
    let high = this.high;
    let low = this.low;
    this.low = (low + 1) >>> 0;
    if (this.low === 0) {
      this.high = (high + 1) >>> 0;
    }

    // bits = (bits ^ (bits >> 30)) * 0xbf58476d1ce4e5b9
    low = (low ^ ((low >>> 30) | (high << 2))) >>> 0;
    high = (high ^ (high >>> 30)) >>> 0;
    let productHigh = SampleIndex.multiplyHigh(low, 0x1ce4e5b9, high, 0xbf58476d);
    let productLow = Math.imul(low, 0x1ce4e5b9) >>> 0;
    high = productHigh;
    low = productLow;

    // bits = (bits ^ (bits >> 27)) * 0x94d049bb133111eb
    low = (low ^ ((low >>> 27) | (high << 5))) >>> 0;
    high = (high ^ (high >>> 27)) >>> 0;
    productHigh = SampleIndex.multiplyHigh(low, 0x133111eb, high, 0x94d049bb);
    productLow = Math.imul(low, 0x133111eb) >>> 0;
    high = productHigh;
    low = productLow;

    // bits ^= bits >> 31
    low = (low ^ ((low >>> 31) | (high << 1))) >>> 0;
    high = (high ^ (high >>> 31)) >>> 0;

    // (bits >> 11) / 2^53: an integer below 2^53, exact as a double
    const top = high >>> 11;
    const bottom = ((low >>> 11) | (high << 21)) >>> 0;
    return (top * 4294967296 + bottom) / 9007199254740992;
  }

  /**
  Upper half of the 64 bit product (high:low) * (constantHigh:constantLow),
  modulo 2^64
  */
  private static multiplyHigh(low: number, constantLow: number, high: number, constantHigh: number): number {
    return (SampleIndex.multiplyHigh32(low, constantLow)
      + Math.imul(low, constantHigh)
      + Math.imul(high, constantLow)) >>> 0;
  }

  /** Upper 32 bits of the 64 bit product of two unsigned 32 bit integers */
  private static multiplyHigh32(a: number, b: number): number {
    const a0 = a & 0xffff;
    const a1 = a >>> 16;
    const b0 = b & 0xffff;
    const b1 = b >>> 16;
    const p00 = a0 * b0;
    const p01 = a0 * b1;
    const p10 = a1 * b0;
    const middle = (p00 >>> 16) + (p01 & 0xffff) + (p10 & 0xffff);
    return (a1 * b1 + Math.floor(p01 / 65536) + Math.floor(p10 / 65536) + Math.floor(middle / 65536)) >>> 0;
  }
}
