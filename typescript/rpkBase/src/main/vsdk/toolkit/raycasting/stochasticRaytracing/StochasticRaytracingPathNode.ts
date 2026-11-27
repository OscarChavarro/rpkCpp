import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Patch } from "../../environment/geometry/elements/Patch";

/**
Path node: contains all necessary data for computing the score afterwards
*/
export class StochasticRaytracingPathNode {
  public patch: Patch | null;
  public probability: number;
  public inPoint: Vector3Dd;
  public outpoint: Vector3Dd;

  public constructor() {
    this.patch = null;
    this.probability = 0.0;
    this.inPoint = new Vector3Dd();
    this.outpoint = new Vector3Dd();
  }
}
