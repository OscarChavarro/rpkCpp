import { Vector2D } from "./Vector2D";

export class Matrix2x2 {
  public m: number[][];
  public t: number[];

  public constructor() {
    this.m = [
      [0.0, 0.0],
      [0.0, 0.0]
    ];
    this.t = [0.0, 0.0];
  }

  public transformPoint2D(src: Vector2D, dst: Vector2D): void {
    const outX = this.m[0][0] * src.x + this.m[0][1] * src.y + this.t[0];
    const outY = this.m[1][0] * src.x + this.m[1][1] * src.y + this.t[1];
    dst.x = outX;
    dst.y = outY;
  }

  public matrix2DPreConcatTransform(xf1: Matrix2x2, xf: Matrix2x2): void {
    const tmpXf = new Matrix2x2();
    tmpXf.m[0][0] = this.m[0][0] * xf1.m[0][0] + this.m[0][1] * xf1.m[1][0];
    tmpXf.m[0][1] = this.m[0][0] * xf1.m[0][1] + this.m[0][1] * xf1.m[1][1];
    tmpXf.m[1][0] = this.m[1][0] * xf1.m[0][0] + this.m[1][1] * xf1.m[1][0];
    tmpXf.m[1][1] = this.m[1][0] * xf1.m[0][1] + this.m[1][1] * xf1.m[1][1];
    tmpXf.t[0] = this.m[0][0] * xf1.t[0] + this.m[0][1] * xf1.t[1] + this.t[0];
    tmpXf.t[1] = this.m[1][0] * xf1.t[0] + this.m[1][1] * xf1.t[1] + this.t[1];

    xf.m[0][0] = tmpXf.m[0][0];
    xf.m[0][1] = tmpXf.m[0][1];
    xf.m[1][0] = tmpXf.m[1][0];
    xf.m[1][1] = tmpXf.m[1][1];
    xf.t[0] = tmpXf.t[0];
    xf.t[1] = tmpXf.t[1];
  }
}
