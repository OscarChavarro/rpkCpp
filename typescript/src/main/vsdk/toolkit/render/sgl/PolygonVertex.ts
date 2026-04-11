export class PolygonVertex {
  public sx: number;
  public sy: number;
  public sz: number;
  public sw: number;
  public x: number;
  public y: number;
  public z: number;
  public u: number;
  public v: number;
  public r: number;
  public g: number;
  public b: number;

  public constructor() {
    this.sx = 0.0;
    this.sy = 0.0;
    this.sz = 0.0;
    this.sw = 0.0;
    this.x = 0.0;
    this.y = 0.0;
    this.z = 0.0;
    this.u = 0.0;
    this.v = 0.0;
    this.r = 0.0;
    this.g = 0.0;
    this.b = 0.0;
  }

  public getCoord(i: number): number {
    switch (i) {
      case 0:
        return this.sx;
      case 1:
        return this.sy;
      case 2:
        return this.sz;
      case 3:
        return this.sw;
      case 4:
        return this.x;
      case 5:
        return this.y;
      case 6:
        return this.z;
      case 7:
        return this.u;
      case 8:
        return this.v;
      case 9:
        return this.r;
      case 10:
        return this.g;
      case 11:
        return this.b;
      default:
        return 0.0;
    }
  }
}
