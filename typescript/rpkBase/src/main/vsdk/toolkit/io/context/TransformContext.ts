import { Matrix4x4d } from "../../common/linealAlgebra/Matrix4x4d";

export class TransformContext {
  public transformMatrix: Matrix4x4d;
  public scaleFactor: number;

  public constructor() {
    this.transformMatrix = new Matrix4x4d();
    this.scaleFactor = 0.0;
  }
}
