import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";

export class DensityHit {
  public m_x: number;
  public m_y: number;
  public color: ColorRgbMutable;

  public constructor();
  public constructor(x: number, y: number, col: ColorRgbMutable);
  public constructor(x?: number, y?: number, col?: ColorRgbMutable) {
    this.m_x = 0.0;
    this.m_y = 0.0;
    this.color = new ColorRgbMutable();

    if (x !== undefined && y !== undefined && col !== undefined) {
      this.init(x, y, col);
    }
  }

  public init(x: number, y: number, col: ColorRgbMutable): void {
    this.m_x = x;
    this.m_y = y;
    this.color = new ColorRgbMutable(col.r, col.g, col.b);
  }
}
