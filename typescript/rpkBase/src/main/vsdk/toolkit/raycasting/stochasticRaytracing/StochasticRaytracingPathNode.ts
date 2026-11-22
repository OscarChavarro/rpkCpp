import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Patch } from "../../environment/geometry/elements/Patch";

/**
Path node: contains all necessary data for computing the score afterwards
*/
export class StochasticRaytracingPathNode {
  public patch: Patch | null;
  public probability: number;
  public inPoint: Vector3Df;
  public outpoint: Vector3Df;

  public constructor() {
    this.patch = null;
    this.probability = 0.0;
    this.inPoint = new Vector3Df();
    this.outpoint = new Vector3Df();
  }
}
