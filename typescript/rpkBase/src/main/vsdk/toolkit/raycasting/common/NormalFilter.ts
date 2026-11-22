import { PixelFilter } from "./PixelFilter";

export class NormalFilter extends PixelFilter {
  public sigma: number;
  public dist: number;

  public constructor();
  public constructor(s: number, d: number);
  public constructor(s?: number, d?: number) {
    super();
    if (s === undefined || d === undefined) {
      this.sigma = 0.70710678;
      this.dist = 2.0;
    }
    else {
      this.sigma = s;
      this.dist = d;
    }
  }

  public override sample(xi1: number[], xi2: number[]): void {
    const s = this.dist / this.sigma;
    const r = xi1[0]! * globalThis.Math.exp(s * s * (-0.5));
    const a = xi2[0]!;

    xi1[0] = this.sigma * (globalThis.Math.sqrt(-2.0 * globalThis.Math.log(r)) * globalThis.Math.cos(2.0 * globalThis.Math.PI * a)) + 0.5;
    xi2[0] = this.sigma * (globalThis.Math.sqrt(-2.0 * globalThis.Math.log(r)) * globalThis.Math.sin(2.0 * globalThis.Math.PI * a)) + 0.5;
  }
}
