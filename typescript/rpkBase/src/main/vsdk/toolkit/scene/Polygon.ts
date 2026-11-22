import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import { Patch } from "../environment/geometry/elements/Patch";

export class Polygon {
  public normal: Vector3Df;
  public planeConstant: number;
  public bounds: BoundingBox;
  public vertex: Vector3Df[];
  public numberOfVertices: number;
  public index: number;

  public constructor() {
    this.normal = new Vector3Df();
    this.planeConstant = 0.0;
    this.bounds = new BoundingBox();
    this.vertex = new Array<Vector3Df>(Patch.MAXIMUM_VERTICES_PER_PATCH);
    for (let i = 0; i < this.vertex.length; i++) {
      this.vertex[i] = new Vector3Df();
    }
    this.numberOfVertices = 0;
    this.index = 0;
  }
}
