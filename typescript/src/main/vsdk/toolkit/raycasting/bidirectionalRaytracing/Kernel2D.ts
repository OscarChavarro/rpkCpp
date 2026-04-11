import { ColorRgb } from "../../common/ColorRgb";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector2D } from "../../common/linealAlgebra/Vector2D";
import { ScreenBuffer } from "../../render/ScreenBuffer";

export class Kernel2D {
  private static readonly M_2_PI = 2.0 / globalThis.Math.PI;

  private m_h: number;
  private m_h2: number;
  private m_h2inv: number;
  private m_weight: number;

  public constructor() {
    this.m_h = 1.0;
    this.m_h2 = 1.0;
    this.m_h2inv = 1.0;
    this.m_weight = 1.0;
    this.Init(1.0, 1.0);
  }

  public Init(h: number, w: number): void {
    this.m_h = h;
    this.m_weight = w;

    this.m_h2 = h * h;
    this.m_h2inv = 1.0 / this.m_h2;
  }

  public SetH(newH: number): void {
    this.Init(newH, this.m_weight);
  }

  public Evaluate(point: Vector2D, center: Vector2D): number {
    const aux = new Vector2D();

    Vector2D.difference(point, center, aux);
    let tp = Vector2D.norm2(aux);

    if (tp < this.m_h2) {
      tp = 1.0 - tp * this.m_h2inv;
      tp = Kernel2D.M_2_PI * tp * this.m_h2inv;
      return tp;
    }
    return 0.0;
  }

  public cover(point: Vector2D, scale: number, col: ColorRgb, screen: ScreenBuffer): void {
    const nxMinV = [0];
    const nxMaxV = [0];
    const nyMinV = [0];
    const nyMaxV = [0];
    const addCol = new ColorRgb();

    screen.getPixel(point.x - this.m_h, point.y - this.m_h, nxMinV, nyMinV);
    screen.getPixel(point.x + this.m_h, point.y + this.m_h, nxMaxV, nyMaxV);

    const nxMin = nxMinV[0];
    const nxMax = nxMaxV[0];
    const nyMin = nyMinV[0];
    const nyMax = nyMaxV[0];

    for (let nx = nxMin; nx <= nxMax; nx++) {
      for (let ny = nyMin; ny <= nyMax; ny++) {
        if (nx >= 0 && ny >= 0 && nx < screen.getHRes() && ny < screen.getVRes()) {
          const center = screen.getPixelCenter(nx, ny);
          const factor = scale * this.Evaluate(point, center);
          addCol.scaledCopy(factor, col);
          screen.add(nx, ny, addCol);
        }
      }
    }
  }

  public varCover(
    center: Vector2D,
    color: ColorRgb,
    ref: ScreenBuffer,
    dest: ScreenBuffer,
    totalSamples: number,
    scaleSamples: number,
    baseSize: number
  ): void {
    const screenScale = globalThis.Math.max(ref.getPixXSize(), ref.getPixYSize());
    const B = baseSize * screenScale;

    const Bn = B * globalThis.Math.pow(scaleSamples, -1.5 / 5.0);

    let h: number;

    const fe = ref.getBiLinear(center.x, center.y);

    const avgFe = fe.average();
    const avgG = color.average();

    if (avgFe > Numeric.EPSILON) {
      h = Bn * globalThis.Math.sqrt(avgG / avgFe);
    }
    else {
      const maxRatio = 20.0;
      h = Bn * maxRatio * screenScale;
      process.stdout.write("MaxRatio... h = " + (h / screenScale) + "\n");
    }

    h = globalThis.Math.max(1.0 * screenScale, h);

    this.SetH(h);

    this.cover(center, 1.0 / totalSamples, color, dest);
  }
}
