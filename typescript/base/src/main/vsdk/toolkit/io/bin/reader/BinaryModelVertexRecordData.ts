import { ColorRgbMutable } from "../../../common/color/ColorRgbMutable";
import { BinaryModelIndexListRef } from "./BinaryModelIndexListRef";

export class BinaryModelVertexRecordData {
  public id: number;
  public pointIndex: number;
  public normalIndex: number;
  public textureCoordinateIndex: number;
  public color: ColorRgbMutable;
  public backIndex: number;
  public tmp: number;
  public hasRadianceData: boolean;
  public patchIndices: BinaryModelIndexListRef;

  public constructor() {
    this.id = 0;
    this.pointIndex = -1;
    this.normalIndex = -1;
    this.textureCoordinateIndex = -1;
    this.color = new ColorRgbMutable();
    this.backIndex = -1;
    this.tmp = 0;
    this.hasRadianceData = false;
    this.patchIndices = new BinaryModelIndexListRef();
  }
}
