import { RadianceMethod } from "../../scene/RadianceMethod";

export class ParseOptionsContext {
  public radianceMethod: RadianceMethod | null;
  public singleSided: boolean;
  public numberOfQuarterCircleDivisions: number;
  public monochrome: boolean;

  public constructor() {
    this.radianceMethod = null;
    this.singleSided = false;
    this.numberOfQuarterCircleDivisions = 0;
    this.monochrome = false;
  }
}
