import { ColorRgb } from "../../common/color/ColorRgb";

export class DensityHit {
  public m_x: number;
  public m_y: number;
  public color: ColorRgb;

  public constructor();
  public constructor(x: number, y: number, col: ColorRgb);
  public constructor(x?: number, y?: number, col?: ColorRgb) {
    this.m_x = 0.0;
    this.m_y = 0.0;
    this.color = new ColorRgb();

    if (x !== undefined && y !== undefined && col !== undefined) {
      this.init(x, y, col);
    }
  }

  public init(x: number, y: number, col: ColorRgb): void {
    this.m_x = x;
    this.m_y = y;
    this.color = new ColorRgb(col.r, col.g, col.b);
  }
}
