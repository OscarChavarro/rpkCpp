import { BinaryModelIndexListRef } from "./BinaryModelIndexListRef";

export class BinaryModelSnapshotRecordData {
  public currentColorIndex: number;
  public hasCurrentMaterialName: boolean;
  public currentMaterialName: string | null;
  public hasCurrentObjectName: boolean;
  public currentObjectName: string | null;
  public hasCurrentVertexName: boolean;
  public currentVertexName: string | null;
  public geometryStackHeadIndex: number;
  public inComplex: boolean;
  public inSurface: boolean;
  public monochrome: boolean;
  public singleSided: boolean;
  public warpConeEnds: boolean;
  public numberOfQuarterCircleDivisions: number;
  public readerContextIndex: number;
  public transformContextIndex: number;

  public currentFaceList: BinaryModelIndexListRef;
  public currentGeometryList: BinaryModelIndexListRef;
  public currentNormalList: BinaryModelIndexListRef;
  public currentPointList: BinaryModelIndexListRef;
  public currentVertexList: BinaryModelIndexListRef;
  public geometries: BinaryModelIndexListRef;
  public materials: BinaryModelIndexListRef;

  public constructor() {
    this.currentColorIndex = 0;
    this.hasCurrentMaterialName = false;
    this.currentMaterialName = null;
    this.hasCurrentObjectName = false;
    this.currentObjectName = null;
    this.hasCurrentVertexName = false;
    this.currentVertexName = null;
    this.geometryStackHeadIndex = 0;
    this.inComplex = false;
    this.inSurface = false;
    this.monochrome = false;
    this.singleSided = false;
    this.warpConeEnds = false;
    this.numberOfQuarterCircleDivisions = 0;
    this.readerContextIndex = 0;
    this.transformContextIndex = 0;

    this.currentFaceList = new BinaryModelIndexListRef();
    this.currentGeometryList = new BinaryModelIndexListRef();
    this.currentNormalList = new BinaryModelIndexListRef();
    this.currentPointList = new BinaryModelIndexListRef();
    this.currentVertexList = new BinaryModelIndexListRef();
    this.geometries = new BinaryModelIndexListRef();
    this.materials = new BinaryModelIndexListRef();
  }
}
