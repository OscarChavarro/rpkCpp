import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { IrrPhoton } from "./IrrPhoton";

export class NormalQuery {
  public photon: IrrPhoton | null;
  public point: number[] | null;
  public normal: Vector3Dd;
  public threshold: number;
  public maximumDistance: number;

  public constructor() {
    this.photon = null;
    this.point = null;
    this.normal = new Vector3Dd();
    this.threshold = 0.0;
    this.maximumDistance = 0.0;
  }
}

