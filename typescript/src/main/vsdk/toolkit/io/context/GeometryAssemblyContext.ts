import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../skin/Patch";
import { Vertex } from "../../skin/Vertex";

export class GeometryAssemblyContext {
  public static readonly MAXIMUM_GEOMETRY_STACK_DEPTH = 100;

  public currentVertexName: string | null;
  public geometryStackHeadIndex: number;
  public geometryStack: Array<Geometry[] | null>;

  public currentPointList: Vector3D[] | null;
  public currentNormalList: Vector3D[] | null;
  public currentVertexList: Vertex[] | null;
  public currentFaceList: Patch[] | null;
  public currentGeometryList: Geometry[] | null;
  public currentObjectName: string | null;
  public inSurface: boolean;
  public inComplex: boolean;
  public warpConeEnds: boolean;
  public allGeometries: Geometry[] | null;

  public geometries: Geometry[] | null;

  public constructor() {
    this.currentVertexName = null;
    this.geometryStackHeadIndex = 0;
    this.geometryStack = new Array<Geometry[] | null>(GeometryAssemblyContext.MAXIMUM_GEOMETRY_STACK_DEPTH).fill(null);
    this.currentPointList = null;
    this.currentNormalList = null;
    this.currentVertexList = null;
    this.currentFaceList = null;
    this.currentGeometryList = null;
    this.currentObjectName = null;
    this.inSurface = false;
    this.inComplex = false;
    this.warpConeEnds = false;
    this.allGeometries = [];
    this.geometries = null;
  }

  public destroy(): void {
    this.currentObjectName = null;
    if (this.allGeometries !== null) {
      this.allGeometries.length = 0;
      this.allGeometries = null;
    }
  }
}
