import { OptionsGroupRaytracingMethod } from "./OptionsGroupRaytracingMethod";

export class OptionsGroupRaytracing {
  private constructor() {
  }

  private static makeMethodsHelpMessage(buffer: string[]): void {
    buffer[0] = "-raytracing-method <method>: set pixel-based radiance computation method\n"
      + "\tmethods: none                 no pixel-based radiance computation\n"
      + "\t         StochasticRaytracing Stochastic Raytracing & Final Gathers (default)\n"
      + "\t         BidirectionalPathTra Bidirectional Path Tracing\n"
      + "\t         RayCasting           Ray Casting\n"
      + "\t         RayMatting           Ray Matting";
  }

  public static parse(argc: number[], argv: string[], rayTracerName: string[]): void {
    const helpMessage = [""];

    OptionsGroupRaytracing.makeMethodsHelpMessage(helpMessage);
    if (rayTracerName !== null && rayTracerName.length > 0) {
      rayTracerName[0] = "none";
    }
    OptionsGroupRaytracingMethod.rayTracingParseOptions(argc, argv, helpMessage, rayTracerName);
  }
}
