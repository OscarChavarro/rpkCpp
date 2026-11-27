import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import { Patch } from "../environment/geometry/elements/Patch";

export class ConvexPolygon {
  public normal: Vector3Dd;
  public planeConstant: number;
  public bounds: BoundingBox;
  public vertex: Vector3Dd[];
  public numberOfVertices: number;
  public index: number;

  public constructor() {
    this.normal = new Vector3Dd();
    this.planeConstant = 0.0;
    this.bounds = new BoundingBox();
    this.vertex = new Array<Vector3Dd>(Patch.MAXIMUM_VERTICES_PER_PATCH);
    for (let i = 0; i < this.vertex.length; i++) {
      this.vertex[i] = new Vector3Dd();
    }
    this.numberOfVertices = 0;
    this.index = 0;
  }
}
