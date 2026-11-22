import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";

export class Plane {
  public normal: Vector3Df;
  public d: number;

  public constructor() {
    this.normal = new Vector3Df();
    this.d = 0.0;
  }
}
