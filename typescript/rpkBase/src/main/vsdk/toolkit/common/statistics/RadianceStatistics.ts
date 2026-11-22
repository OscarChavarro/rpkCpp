import { ColorRgbMutable } from "../color/ColorRgbMutable";

export class RadianceStatistics {
  public totalArea: number;
  public maxSelfEmittedRadiance: ColorRgbMutable;
  public maxSelfEmittedPower: ColorRgbMutable;
  public referenceLuminance: number;
  public totalEmittedPower: ColorRgbMutable;
  public estimatedAverageRadiance: ColorRgbMutable;
  public averageReflectivity: ColorRgbMutable;

  public constructor() {
    this.totalArea = 0.0;
    this.maxSelfEmittedRadiance = new ColorRgbMutable();
    this.maxSelfEmittedPower = new ColorRgbMutable();
    this.referenceLuminance = 0.0;
    this.totalEmittedPower = new ColorRgbMutable();
    this.estimatedAverageRadiance = new ColorRgbMutable();
    this.averageReflectivity = new ColorRgbMutable();
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
