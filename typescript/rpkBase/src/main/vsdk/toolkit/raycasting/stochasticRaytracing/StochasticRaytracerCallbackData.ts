import { RendererConfiguration } from "../../material/RendererConfiguration";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { StochasticRaytracingConfiguration } from "./StochasticRaytracingConfiguration";

export class StochasticRaytracerCallbackData {
  public constructor(
    public readonly config: StochasticRaytracingConfiguration,
    public readonly radianceMethod: RadianceMethod,
    public readonly renderOptions: RendererConfiguration
  ) {}
}
