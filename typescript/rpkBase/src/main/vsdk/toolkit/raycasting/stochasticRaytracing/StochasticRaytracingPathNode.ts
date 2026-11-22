import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Patch } from "../../environment/geometry/elements/Patch";

/**
Path node: contains all necessary data for computing the score afterwards
*/
export class StochasticRaytracingPathNode {
  public patch: Patch | null;
  public probability: number;
  public inPoint: Vector3D;
  public outpoint: Vector3D;

  public constructor() {
    this.patch = null;
    this.probability = 0.0;
    this.inPoint = new Vector3D();
    this.outpoint = new Vector3D();
  }
}
