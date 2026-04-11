import { RayMatterFilterType } from "./RayMatterFilterType";

export class RayMatterState {
  public samplesPerPixel: number;
  public filter: RayMatterFilterType;

  public constructor() {
    this.samplesPerPixel = 8;
    this.filter = RayMatterFilterType.TENT_FILTER;
  }
}
