export class BidirectionalPathRaytracerConfig {
  public static readonly MAX_REGEXP_SIZE = 100;

  public maximumEyePathDepth: number;
  public maximumLightPathDepth: number;
  public maximumPathDepth: number;
  public minimumPathDepth: number;

  public samplesPerPixel: number;
  public totalSamples: number;
  public sampleImportantLights: number;
  public progressiveTracing: number;
  public eliminateSpikes: number;

  public useSpars: number;
  public doLe: number;
  public doLD: number;
  public doLI: number;

  public leRegExp: string;
  public ldRegExp: string;
  public liRegExp: string;

  public doWeighted: number;
  public wleRegExp: string;
  public wldRegExp: string;

  public doDensityEstimation: number;

  public constructor() {
    this.maximumEyePathDepth = 0;
    this.maximumLightPathDepth = 0;
    this.maximumPathDepth = 0;
    this.minimumPathDepth = 0;

    this.samplesPerPixel = 0;
    this.totalSamples = 0;
    this.sampleImportantLights = 0;
    this.progressiveTracing = 0;
    this.eliminateSpikes = 0;

    this.useSpars = 0;
    this.doLe = 0;
    this.doLD = 0;
    this.doLI = 0;

    this.leRegExp = "";
    this.ldRegExp = "";
    this.liRegExp = "";

    this.doWeighted = 0;
    this.wleRegExp = "";
    this.wldRegExp = "";

    this.doDensityEstimation = 0;
  }

  public copyFrom(other: BidirectionalPathRaytracerConfig | null): void {
    if (other === null) {
      return;
    }
    this.maximumEyePathDepth = other.maximumEyePathDepth;
    this.maximumLightPathDepth = other.maximumLightPathDepth;
    this.maximumPathDepth = other.maximumPathDepth;
    this.minimumPathDepth = other.minimumPathDepth;
    this.samplesPerPixel = other.samplesPerPixel;
    this.totalSamples = other.totalSamples;
    this.sampleImportantLights = other.sampleImportantLights;
    this.progressiveTracing = other.progressiveTracing;
    this.eliminateSpikes = other.eliminateSpikes;
    this.useSpars = other.useSpars;
    this.doLe = other.doLe;
    this.doLD = other.doLD;
    this.doLI = other.doLI;
    this.leRegExp = other.leRegExp;
    this.ldRegExp = other.ldRegExp;
    this.liRegExp = other.liRegExp;
    this.doWeighted = other.doWeighted;
    this.wleRegExp = other.wleRegExp;
    this.wldRegExp = other.wldRegExp;
    this.doDensityEstimation = other.doDensityEstimation;
  }
}
