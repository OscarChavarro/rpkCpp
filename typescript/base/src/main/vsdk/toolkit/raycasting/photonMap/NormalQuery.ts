import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { IrrPhoton } from "./IrrPhoton";

export class NormalQuery {
  public photon: IrrPhoton | null;
  public point: number[] | null;
  public normal: Vector3D;
  public threshold: number;
  public maximumDistance: number;

  public constructor() {
    this.photon = null;
    this.point = null;
    this.normal = new Vector3D();
    this.threshold = 0.0;
    this.maximumDistance = 0.0;
  }
}

