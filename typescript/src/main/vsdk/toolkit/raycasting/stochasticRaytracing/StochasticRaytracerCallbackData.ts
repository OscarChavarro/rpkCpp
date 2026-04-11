import { RenderOptions } from "../../common/RenderOptions";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { StochasticRaytracingConfiguration } from "./StochasticRaytracingConfiguration";

export class StochasticRaytracerCallbackData {
  public config: StochasticRaytracingConfiguration | null;
  public radianceMethod: RadianceMethod | null;
  public renderOptions: RenderOptions | null;

  public constructor() {
    this.config = null;
    this.radianceMethod = null;
    this.renderOptions = null;
  }
}
