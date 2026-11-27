import { Cie } from "vitral/dist/vsdk/toolkit/common/color/Cie";
import { RadianceMethod } from "../../scene/RadianceMethod";

export class ParseOptionsContext {
  public radianceMethod: RadianceMethod | null;
  public singleSided: boolean;
  public numberOfQuarterCircleDivisions: number;
  public monochrome: boolean;

  // RGB color space MGF colors (CIE xy) are converted into
  private colorSpace: Cie;

  public constructor() {
    this.radianceMethod = null;
    this.singleSided = false;
    this.numberOfQuarterCircleDivisions = 0;
    this.monochrome = false;
    this.colorSpace = new Cie();
  }

  public getColorSpace(): Cie {
    return this.colorSpace;
  }

  public setColorSpace(inColorSpace: Cie): void {
    this.colorSpace = inColorSpace;
  }
}
