import { BinaryModelIndexListRef } from "./BinaryModelIndexListRef";

export class BinaryModelGeometryRecordData {
  public classId: number;
  public id: number;
  public itemCount: number;
  public bounded: boolean;
  public shaftCullGeometry: boolean;
  public omit: boolean;
  public isDuplicate: boolean;
  public boundingBoxCoordinates: number[];
  public hasRayIntersectionBox: boolean;
  public hasRadianceData: boolean;

  public hasObjectName: boolean;
  public objectName: string | null;
  public meshId: number;
  public materialIndex: number;
  public positions: BinaryModelIndexListRef;
  public normals: BinaryModelIndexListRef;
  public vertices: BinaryModelIndexListRef;
  public faces: BinaryModelIndexListRef;

  public children: BinaryModelIndexListRef;
  public patchSetPatches: BinaryModelIndexListRef;

  public constructor() {
    this.classId = 0;
    this.id = 0;
    this.itemCount = 0;
    this.bounded = false;
    this.shaftCullGeometry = false;
    this.omit = false;
    this.isDuplicate = false;
    this.boundingBoxCoordinates = new Array<number>(6).fill(0.0);
    this.hasRayIntersectionBox = false;
    this.hasRadianceData = false;

    this.hasObjectName = false;
    this.objectName = null;
    this.meshId = 0;
    this.materialIndex = -1;
    this.positions = new BinaryModelIndexListRef();
    this.normals = new BinaryModelIndexListRef();
    this.vertices = new BinaryModelIndexListRef();
    this.faces = new BinaryModelIndexListRef();

    this.children = new BinaryModelIndexListRef();
    this.patchSetPatches = new BinaryModelIndexListRef();
  }
}
