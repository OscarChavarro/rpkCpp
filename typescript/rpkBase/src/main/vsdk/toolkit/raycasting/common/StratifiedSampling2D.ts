import { Random48 } from "../../common/Random48";

export class StratifiedSampling2D {
  private readonly xMaxStratum: number;
  private readonly yMaxStratum: number;
  private xStratum: number;
  private yStratum: number;

  public constructor(nrSamples: number) {
    const divs1 = [0];
    const divs2 = [0];
    StratifiedSampling2D.getNumberOfDivisions(nrSamples, divs1, divs2);
    this.xMaxStratum = divs1[0]!;
    this.yMaxStratum = divs2[0]!;
    this.xStratum = 0;
    this.yStratum = 0;
  }

  public sample(x1: number[], x2: number[]): void;
  public sample(): number[];
  public sample(x1?: number[], x2?: number[]): void | number[] {
    if (x1 === undefined || x2 === undefined) {
      const sx1 = [0.0];
      const sx2 = [0.0];
      this.sample(sx1, sx2);
      return [sx1[0]!, sx2[0]!];
    }

    if (x1.length === 0 || x2.length === 0) {
      throw new Error("Output arrays must have length >= 1");
    }

    if (this.yStratum < this.yMaxStratum && this.xMaxStratum > 0 && this.yMaxStratum > 0) {
      x1[0] = (this.xStratum + Random48.drand48()) / this.xMaxStratum;
      x2[0] = (this.yStratum + Random48.drand48()) / this.yMaxStratum;

      this.xStratum++;
      if (this.xStratum === this.xMaxStratum) {
        this.xStratum = 0;
        this.yStratum++;
      }
    }
    else {
      x1[0] = Random48.drand48();
      x2[0] = Random48.drand48();
    }
  }

  private static getNumberOfDivisions(samples: number, divs1: number[], divs2: number[]): void {
    if (samples <= 0) {
      divs1[0] = 0;
      divs2[0] = 0;
      return;
    }

    divs1[0] = globalThis.Math.ceil(globalThis.Math.sqrt(samples));
    divs2[0] = globalThis.Math.floor(samples / divs1[0]);
    while (divs1[0] * divs2[0] !== samples && divs1[0] > 1) {
      divs1[0]--;
      divs2[0] = globalThis.Math.floor(samples / divs1[0]);
    }
  }
}
