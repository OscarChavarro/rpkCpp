import { ColorRgb } from "../../common/color/ColorRgb";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector2D } from "../../common/linealAlgebra/Vector2D";
import { ScreenBuffer } from "../../render/ScreenBuffer";
import { BidirectionalPathRaytracerConfig } from "./BidirectionalPathRaytracerConfig";
import { DensityHit } from "./DensityHit";
import { DensityHitList } from "./DensityHitList";
import { Kernel2D } from "./Kernel2D";

export class DensityBuffer {
  private static readonly DHA_X_RES = 50;
  private static readonly DHA_Y_RES = 50;

  private screenBuffer: ScreenBuffer;
  private baseConfig: BidirectionalPathRaytracerConfig;
  private xMinimum: number;
  private xMaximum: number;
  private yMinimum: number;
  private yMaximum: number;
  private hitGrid: DensityHitList[][];

  private xIndex(x: number): number {
    return globalThis.Math.min(
      globalThis.Math.trunc(DensityBuffer.DHA_X_RES * (x - this.xMinimum) / (this.xMaximum - this.xMinimum)),
      DensityBuffer.DHA_X_RES - 1
    );
  }

  private yIndex(y: number): number {
    return globalThis.Math.min(
      globalThis.Math.trunc(DensityBuffer.DHA_Y_RES * (y - this.yMinimum) / (this.yMaximum - this.yMinimum)),
      DensityBuffer.DHA_Y_RES - 1
    );
  }

  public constructor(screen: ScreenBuffer, paramBaseConfig: BidirectionalPathRaytracerConfig) {
    this.screenBuffer = screen;
    this.baseConfig = paramBaseConfig;

    this.xMinimum = this.screenBuffer.getScreenXMin();
    this.xMaximum = this.screenBuffer.getScreenXMax();
    this.yMinimum = this.screenBuffer.getScreenYMin();
    this.yMaximum = this.screenBuffer.getScreenYMax();

    this.hitGrid = new Array<DensityHitList[]>(DensityBuffer.DHA_X_RES);
    for (let i = 0; i < DensityBuffer.DHA_X_RES; i++) {
      this.hitGrid[i] = new Array<DensityHitList>(DensityBuffer.DHA_Y_RES);
      for (let j = 0; j < DensityBuffer.DHA_Y_RES; j++) {
        this.hitGrid[i][j] = new DensityHitList();
      }
    }

    process.stdout.write(
      "Density Buffer :\nXmin " + this.xMinimum + ", Ymin " + this.yMinimum + ", Xmax " + this.xMaximum + ", Ymax " + this.yMaximum + "\n"
    );
  }

  public add(x: number, y: number, color: ColorRgb): void {
    const factor = this.screenBuffer.getPixXSize() * this.screenBuffer.getPixYSize() * this.baseConfig.totalSamples;
    const tmpCol = new ColorRgb();

    if (color.average() > Numeric.EPSILON) {
      tmpCol.scaledCopy(factor, color);

      const hit = new DensityHit(x, y, tmpCol);

      this.hitGrid[this.xIndex(x)][this.yIndex(y)].add(hit);
    }
  }

  public reconstruct(): ScreenBuffer {
    const h = 8.0 * globalThis.Math.max(this.screenBuffer.getPixXSize(), this.screenBuffer.getPixYSize())
      / globalThis.Math.sqrt(this.baseConfig.samplesPerPixel);

    process.stdout.write("h = " + h + "\n");

    this.screenBuffer.scaleRadiance(0.0);

    const kernel = new Kernel2D();
    const center = new Vector2D();

    kernel.SetH(h);

    for (let i = 0; i < DensityBuffer.DHA_X_RES; i++) {
      for (let j = 0; j < DensityBuffer.DHA_Y_RES; j++) {
        const maxK = this.hitGrid[i][j].storedHits();

        for (let k = 0; k < maxK; k++) {
          const hit = this.hitGrid[i][j].get(k);

          center.x = hit.m_x;
          center.y = hit.m_y;

          kernel.cover(center, 1.0 / this.baseConfig.totalSamples, hit.color, this.screenBuffer);
        }
      }
    }

    return this.screenBuffer;
  }

  public reconstructVariable(dest: ScreenBuffer, baseSize = 4.0): ScreenBuffer {
    dest.scaleRadiance(0.0);

    const kernel = new Kernel2D();
    const center = new Vector2D();

    for (let i = 0; i < DensityBuffer.DHA_X_RES; i++) {
      for (let j = 0; j < DensityBuffer.DHA_Y_RES; j++) {
        const maxK = this.hitGrid[i][j].storedHits();

        for (let k = 0; k < maxK; k++) {
          const hit = this.hitGrid[i][j].get(k);

          center.x = hit.m_x;
          center.y = hit.m_y;

          kernel.varCover(
            center,
            hit.color,
            this.screenBuffer,
            dest,
            this.baseConfig.totalSamples,
            this.baseConfig.samplesPerPixel,
            baseSize
          );
        }
      }
    }

    return dest;
  }
}
