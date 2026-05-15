import { ScreenBuffer } from "../../render/ScreenBuffer";
import { BidirectionalPathRaytracerConfig } from "./BidirectionalPathRaytracerConfig";

export class BidirectionalPathTracingState {
  public baseConfig: BidirectionalPathRaytracerConfig;
  public lastScreen: ScreenBuffer | null;
  public baseFilename: string;
  public saveSubsequentImages: number;

  public constructor() {
    this.baseConfig = new BidirectionalPathRaytracerConfig();
    this.lastScreen = null;
    this.saveSubsequentImages = 0;

    this.baseConfig.samplesPerPixel = 1;
    this.baseConfig.progressiveTracing = 1;
    this.baseConfig.minimumPathDepth = 2;
    this.baseConfig.maximumPathDepth = 7;
    this.baseConfig.maximumEyePathDepth = 7;
    this.baseConfig.maximumLightPathDepth = 7;
    this.baseConfig.sampleImportantLights = 1;
    this.baseConfig.useSpars = 0;
    this.baseConfig.doLe = 1;
    this.baseConfig.doLD = 0;
    this.baseConfig.doLI = 0;

    this.baseConfig.doWeighted = 0;

    this.baseConfig.leRegExp = "(LX)(X)*(EX)";
    this.baseConfig.ldRegExp = "(LX)(G|S)(X)*(EX),(LX)(EX)";
    this.baseConfig.liRegExp = "(LX)(G|S)(X)*(EX),(LX)(EX)";
    this.baseConfig.wleRegExp = "(LX)(DR)(X)*(EX)";
    this.baseConfig.wldRegExp = "(LX)(X)*(EX)";

    this.baseConfig.eliminateSpikes = 0;
    this.baseConfig.doDensityEstimation = 0;
    this.baseFilename = "";
  }
}
