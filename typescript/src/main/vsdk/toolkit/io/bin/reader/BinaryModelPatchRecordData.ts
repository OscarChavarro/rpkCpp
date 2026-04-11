import { ColorRgb } from "../../../common/ColorRgb";
import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { Patch } from "../../../skin/Patch";

export class BinaryModelPatchRecordData {
  public id: number;
  public twinIndex: number;
  public numberOfVertices: number;
  public vertexIndices: number[];
  public hasBoundingBox: boolean;
  public boundingBoxCoordinates: number[];
  public normal: Vector3D;
  public planeConstant: number;
  public tolerance: number;
  public area: number;
  public midPoint: Vector3D;
  public hasJacobian: boolean;
  public jacobianA: number;
  public jacobianB: number;
  public jacobianC: number;
  public directPotential: number;
  public dominantIndex: number;
  public omit: boolean;
  public flags: number;
  public color: ColorRgb;
  public materialIndex: number;
  public hasRadianceData: boolean;

  public constructor() {
    this.id = 0;
    this.twinIndex = -1;
    this.numberOfVertices = 0;
    this.vertexIndices = new Array<number>(Patch.MAXIMUM_VERTICES_PER_PATCH).fill(-1);
    this.hasBoundingBox = false;
    this.boundingBoxCoordinates = new Array<number>(6).fill(0.0);
    this.normal = new Vector3D();
    this.planeConstant = 0.0;
    this.tolerance = 0.0;
    this.area = 0.0;
    this.midPoint = new Vector3D();
    this.hasJacobian = false;
    this.jacobianA = 0.0;
    this.jacobianB = 0.0;
    this.jacobianC = 0.0;
    this.directPotential = 0.0;
    this.dominantIndex = 0;
    this.omit = false;
    this.flags = 0;
    this.color = new ColorRgb();
    this.materialIndex = -1;
    this.hasRadianceData = false;
  }
}
