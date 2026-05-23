import { PixelFilter } from "./PixelFilter";

export class TentFilter extends PixelFilter {
  public constructor() {
    super();
  }

  public override sample(xi1: number[], xi2: number[]): void {
    const x = globalThis.Math.abs(2.0 * xi1[0]! - 1.0);
    const sx = xi1[0]! < 0.5 ? -1.0 : +1.0;
    const y = globalThis.Math.abs(2.0 * xi2[0]! - 1.0);
    const sy = xi2[0]! < 0.5 ? -1.0 : +1.0;

    if (x > y) {
      xi1[0] = sx * globalThis.Math.sqrt(x) + 0.5;
      xi2[0] = xi1[0] * y + 0.5;
    }
    else {
      xi2[0] = sy * globalThis.Math.sqrt(y) + 0.5;
      xi1[0] = xi2[0] * x + 0.5;
    }
  }
}
