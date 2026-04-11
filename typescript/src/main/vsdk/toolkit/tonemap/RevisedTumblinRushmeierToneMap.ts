import { Cie } from "../common/Cie";
import { ColorRgb } from "../common/ColorRgb";
import { ToneMap } from "./ToneMap";
import { ToneMappingContext } from "./ToneMappingContext";

export class RevisedTumblinRushmeierToneMap extends ToneMap {
  private g: number;
  private comp: number;
  private display: number;
  private lwaRTR: number;
  private ldaRTR: number;

  private static stevensGamma(lum: number): number {
    if (lum > 100.0) {
      return 2.655;
    }
    return 1.855 + 0.4 * globalThis.Math.log10(lum + 2.3e-5);
  }

  public constructor() {
    super();
    this.g = 0.0;
    this.comp = 0.0;
    this.display = 0.0;
    this.lwaRTR = 0.0;
    this.ldaRTR = 0.0;
  }

  public override init(toneMapOptions: ToneMappingContext): void {
    const lwa = toneMapOptions.realWorldAdaptionLuminance;
    const maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    const maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    this.ldaRTR = maximumDisplayLuminance / globalThis.Math.sqrt(maximumDisplayContrast);

    this.g = RevisedTumblinRushmeierToneMap.stevensGamma(lwa) / RevisedTumblinRushmeierToneMap.stevensGamma(this.ldaRTR);
    const gwd = RevisedTumblinRushmeierToneMap.stevensGamma(lwa) / (1.855 + 0.4 * globalThis.Math.log10(this.ldaRTR));
    this.comp = globalThis.Math.pow(globalThis.Math.sqrt(maximumDisplayContrast), gwd - 1) * this.ldaRTR;
    this.display = this.comp / maximumDisplayLuminance;
  }

  public override scaleForComputations(radiance: ColorRgb): ColorRgb {
    const rwl = radiance.luminance();
    let scale: number;

    if (rwl > 0.0) {
      scale = this.comp * globalThis.Math.pow(rwl / this.lwaRTR, this.g) / rwl;
    }
    else {
      scale = 0.0;
    }

    radiance.scale(scale);
    return radiance;
  }

  public override scaleForDisplay(radiance: ColorRgb): ColorRgb {
    const rwl = globalThis.Math.PI * radiance.luminance();
    const eff = Cie.getLuminousEfficacy();
    radiance.scale(eff * globalThis.Math.PI);

    let scale: number;
    if (rwl > 0.0) {
      scale = this.display * globalThis.Math.pow(rwl / this.lwaRTR, this.g) / rwl;
    }
    else {
      scale = 0.0;
    }

    radiance.scale(scale);
    return radiance;
  }
}
