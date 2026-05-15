/**
Options and runtime state for stochastic raytracing.
*/

import { ScreenBuffer } from "../../render/ScreenBuffer";
import { RayTracingLightMode } from "./RayTracingLightMode";
import { RayTracingRadMode } from "./RayTracingRadMode";
import { RayTracingSamplingMode } from "./RayTracingSamplingMode";

export class StochasticRayTracingState {
  public samplesPerPixel: number;
  public progressiveTracing: number;

  public doFrameCoherent: number;
  public doCorrelatedSampling: number;
  public baseSeed: number;

  public radMode: RayTracingRadMode;

  public nextEvent: number;
  public nextEventSamples: number;
  public lightMode: RayTracingLightMode;

  public backgroundDirect: number;
  public backgroundIndirect: number;
  public backgroundSampling: number;

  public scatterSamples: number;
  public differentFirstDG: number;
  public firstDGSamples: number;
  public separateSpecular: number;
  public reflectionSampling: RayTracingSamplingMode;

  public minPathDepth: number;
  public maxPathDepth: number;

  public lastScreen: ScreenBuffer | null;

  public constructor() {
    this.samplesPerPixel = 1;
    this.progressiveTracing = 1;
    this.doFrameCoherent = 0;
    this.doCorrelatedSampling = 0;
    this.baseSeed = 0xFE062134;
    this.radMode = RayTracingRadMode.STORED_NONE;
    this.nextEvent = 1;
    this.nextEventSamples = 1;
    this.lightMode = RayTracingLightMode.ALL_LIGHTS;
    this.backgroundDirect = 0;
    this.backgroundIndirect = 1;
    this.backgroundSampling = 0;
    this.scatterSamples = 1;
    this.differentFirstDG = 0;
    this.firstDGSamples = 36;
    this.separateSpecular = 0;
    this.reflectionSampling = RayTracingSamplingMode.BRDF_SAMPLING;
    this.minPathDepth = 5;
    this.maxPathDepth = 7;
    this.lastScreen = null;
  }
}
