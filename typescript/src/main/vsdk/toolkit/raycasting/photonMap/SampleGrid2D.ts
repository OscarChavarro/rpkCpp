/**
Class for doing multidimensional discrete sampling.
Grid values are doubles. Grid domain is [0,1]^dim
*/

import { Numeric } from "../../common/linealAlgebra/Numeric";
import { DiscreteSampling } from "./DiscreteSampling";

export class SampleGrid2D {
  private xSections: number;
  private ySections: number;
  private values: number[];
  private ySums: number[];
  private totalSum: number;

  private valIndex(i: number, j: number): number {
    return i * this.ySections + j;
  }

  public constructor(xSectionsParam: number, ySectionsParam: number) {
    this.totalSum = 0.0;
    this.xSections = xSectionsParam;
    this.ySections = ySectionsParam;

    this.values = new Array<number>(this.xSections * this.ySections).fill(0.0);
    this.ySums = new Array<number>(this.xSections).fill(0.0);

    this.init();
  }

  public init(): void {
    let index = 0;

    for (let i = 0; i < this.xSections; i++) {
      this.ySums[i] = 0.0;
      for (let j = 0; j < this.ySections; j++) {
        this.values[index++] = 0.0;
      }
    }

    this.totalSum = 0.0;
  }

  public add(x: number, y: number, value: number): void {
    let xIndex = globalThis.Math.trunc(x * this.xSections);
    let yIndex = globalThis.Math.trunc(y * this.ySections);

    if (xIndex === this.xSections) {
      xIndex--;
    }
    if (yIndex === this.ySections) {
      yIndex--;
    }

    this.values[this.valIndex(xIndex, yIndex)] += value;
    this.ySums[xIndex] += value;
    this.totalSum += value;
  }

  public EnsureNonZeroEntries(): void {
    let index = 0;
    const fraction = 0.03 * this.totalSum / (this.xSections * this.ySections);
    const threshold = 1e-10 * this.totalSum;

    for (let i = 0; i < this.xSections; i++) {
      for (let j = 0; j < this.ySections; j++) {
        if (this.values[index] < threshold) {
          this.values[index] += fraction;
          this.ySums[i] += fraction;
          this.totalSum += fraction;
        }
        index++;
      }
    }
  }

  public sample(x: number[], y: number[], probabilityDensityFunction: number[]): void {
    let xIndex: number;
    let yIndex: number;
    const xPdf = [0.0];
    const yPdf = [0.0];

    if (this.totalSum < Numeric.EPSILON) {
      probabilityDensityFunction[0] = 1.0;
      return;
    }

    xIndex = DiscreteSampling.sample(this.ySums, this.totalSum, x, xPdf);

    const row = new Array<number>(this.ySections).fill(0.0);
    for (let j = 0; j < this.ySections; j++) {
      row[j] = this.values[this.valIndex(xIndex, j)];
    }
    yIndex = DiscreteSampling.sample(row, this.ySums[xIndex], y, yPdf);

    probabilityDensityFunction[0] = xPdf[0] * yPdf[0];

    let range = 1.0 / this.xSections;
    x[0] = (x[0] + xIndex) * range;
    probabilityDensityFunction[0] /= range;

    range = 1.0 / this.ySections;
    y[0] = (y[0] + yIndex) * range;
    probabilityDensityFunction[0] /= range;
  }
}

