import { ColorRgb } from "../ColorRgb";

export class RadianceStatistics {
  public totalArea: number;
  public maxSelfEmittedRadiance: ColorRgb;
  public maxSelfEmittedPower: ColorRgb;
  public referenceLuminance: number;
  public totalEmittedPower: ColorRgb;
  public estimatedAverageRadiance: ColorRgb;
  public averageReflectivity: ColorRgb;

  public constructor() {
    this.totalArea = 0.0;
    this.maxSelfEmittedRadiance = new ColorRgb();
    this.maxSelfEmittedPower = new ColorRgb();
    this.referenceLuminance = 0.0;
    this.totalEmittedPower = new ColorRgb();
    this.estimatedAverageRadiance = new ColorRgb();
    this.averageReflectivity = new ColorRgb();
  }

  public reset(): void {
    this.totalArea = 0.0;
    this.maxSelfEmittedRadiance.clear();
    this.maxSelfEmittedPower.clear();
    this.referenceLuminance = 0.0;
    this.totalEmittedPower.clear();
    this.estimatedAverageRadiance.clear();
    this.averageReflectivity.clear();
  }
}
