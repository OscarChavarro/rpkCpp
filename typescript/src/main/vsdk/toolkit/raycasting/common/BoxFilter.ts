import { PixelFilter } from "./PixelFilter";

export class BoxFilter extends PixelFilter {
  public constructor() {
    super();
  }

  public override sample(dx: number[], dy: number[]): void {
    // Box filter keeps the original (dx, dy) point.
    void dx;
    void dy;
  }
}
