export class ShaftPlane {
  public n: number[];
  public d: number;
  public coordinateOffset: number[];

  public constructor() {
    this.n = new Array<number>(3).fill(0.0);
    this.d = 0.0;
    this.coordinateOffset = new Array<number>(3).fill(0);
  }
}
