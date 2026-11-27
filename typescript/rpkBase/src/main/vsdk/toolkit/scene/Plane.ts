import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";

export class Plane {
  public normal: Vector3Dd;
  public d: number;

  public constructor() {
    this.normal = new Vector3Dd();
    this.d = 0.0;
  }
}
