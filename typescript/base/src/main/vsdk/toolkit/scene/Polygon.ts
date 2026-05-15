import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import { Patch } from "../environment/geometry/elements/Patch";

export class Polygon {
  public normal: Vector3D;
  public planeConstant: number;
  public bounds: BoundingBox;
  public vertex: Vector3D[];
  public numberOfVertices: number;
  public index: number;

  public constructor() {
    this.normal = new Vector3D();
    this.planeConstant = 0.0;
    this.bounds = new BoundingBox();
    this.vertex = new Array<Vector3D>(Patch.MAXIMUM_VERTICES_PER_PATCH);
    for (let i = 0; i < this.vertex.length; i++) {
      this.vertex[i] = new Vector3D();
    }
    this.numberOfVertices = 0;
    this.index = 0;
  }
}
