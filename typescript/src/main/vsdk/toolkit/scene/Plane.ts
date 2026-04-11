import { Vector3D } from "../common/linealAlgebra/Vector3D";

export class Plane {
  public normal: Vector3D;
  public d: number;

  public constructor() {
    this.normal = new Vector3D();
    this.d = 0.0;
  }
}
