import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Material } from "../../material/Material";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../environment/geometry/elements/Patch";
import { Vertex } from "../../environment/geometry/elements/Vertex";
import { ColorContext } from "./ColorContext";
import { ReaderContext } from "./ReaderContext";
import { TransformStackContext } from "./TransformStackContext";

export class ParseSnapshotContext {
  public currentColor: ColorContext | null;
  public currentFaceList: Patch[] | null;
  public currentGeometryList: Geometry[] | null;
  public currentMaterialName: string | null;
  public currentNormalList: Vector3Dd[] | null;
  public currentObjectName: string | null;
  public currentPointList: Vector3Dd[] | null;
  public currentVertexList: Vertex[] | null;
  public currentVertexName: string | null;
  public geometries: Geometry[] | null;
  public geometryStackHeadIndex: number;
  public inComplex: boolean;
  public inSurface: boolean;
  public materials: Material[] | null;
  public monochrome: boolean;
  public singleSided: boolean;
  public numberOfQuarterCircleDivisions: number;
  public readerContext: ReaderContext | null;
  public transformContext: TransformStackContext | null;

  public constructor() {
    this.currentColor = null;
    this.currentFaceList = null;
    this.currentGeometryList = null;
    this.currentMaterialName = null;
    this.currentNormalList = null;
    this.currentObjectName = null;
    this.currentPointList = null;
    this.currentVertexList = null;
    this.currentVertexName = null;
    this.geometries = null;
    this.geometryStackHeadIndex = 0;
    this.inComplex = false;
    this.inSurface = false;
    this.materials = null;
    this.monochrome = false;
    this.singleSided = false;
    this.numberOfQuarterCircleDivisions = 0;
    this.readerContext = null;
    this.transformContext = null;
  }
}
