export class PolygonBox {
  public x0: number;
  public x1: number;
  public y0: number;
  public y1: number;
  public z0: number;
  public z1: number;

  public constructor();
  public constructor(x0: number, x1: number, y0: number, y1: number, z0: number, z1: number);
  public constructor(x0?: number, x1?: number, y0?: number, y1?: number, z0?: number, z1?: number) {
    this.x0 = x0 ?? 0.0;
    this.x1 = x1 ?? 0.0;
    this.y0 = y0 ?? 0.0;
    this.y1 = y1 ?? 0.0;
    this.z0 = z0 ?? 0.0;
    this.z1 = z1 ?? 0.0;
  }
}
