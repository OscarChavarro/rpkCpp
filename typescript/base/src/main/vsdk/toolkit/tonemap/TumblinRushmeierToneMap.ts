import { Cie } from "../common/color/Cie";
import { ColorRgb } from "../common/color/ColorRgb";
import { ToneMap } from "./ToneMap";
import { ToneMappingContext } from "./ToneMappingContext";

export class TumblinRushmeierToneMap extends ToneMap {
  private invCMaximum: number;
  private lrwmComp: number;
  private lrwmDisplay: number;
  private lrwExponent: number;
  private lda: number;

  public constructor() {
    super();
    this.invCMaximum = 0.0;
    this.lrwmComp = 0.0;
    this.lrwmDisplay = 0.0;
    this.lrwExponent = 0.0;
    this.lda = 0.0;
  }

  public override init(toneMapOptions: ToneMappingContext): void {
    const lwa = toneMapOptions.realWorldAdaptionLuminance;
    const maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    const maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    this.lda = maximumDisplayLuminance / globalThis.Math.sqrt(maximumDisplayContrast);

    let l10 = globalThis.Math.log10(ToneMap.tmoCandelaLambert(lwa));
    const alpha = 0.4 * l10 + 2.92;
    const beta = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;

    l10 = globalThis.Math.log10(ToneMap.tmoCandelaLambert(this.lda));
    const alphaD = 0.4 * l10 + 2.92;
    const betaD = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;

    this.lrwExponent = alpha / alphaD;
    this.lrwmComp = globalThis.Math.pow(10.0, (beta - betaD) / alphaD);
    this.lrwmDisplay = this.lrwmComp / ToneMap.tmoCandelaLambert(maximumDisplayLuminance);
    this.invCMaximum = 1.0 / maximumDisplayContrast;
  }

  public override scaleForComputations(radiance: ColorRgb): ColorRgb {
    const rwl = Cie.spectrumLuminance(radiance.r, radiance.g, radiance.b);

    let scale: number;
    if (rwl > 0.0) {
      const m = ToneMap.tmoLambertCandela(globalThis.Math.pow(ToneMap.tmoCandelaLambert(rwl), this.lrwExponent) * this.lrwmComp);
      scale = m > 0.0 ? m / rwl : 0.0;
    }
    else {
      scale = 0.0;
    }

    radiance.scale(scale);
    return radiance;
  }

  public override scaleForDisplay(radiance: ColorRgb): ColorRgb {
    const rwl = globalThis.Math.PI * Cie.spectrumLuminance(radiance.r, radiance.g, radiance.b);
    const eff = Cie.getLuminousEfficacy();
    radiance.scale(eff * globalThis.Math.PI);

    let scale: number;
    if (rwl > 0.0) {
      const m = (globalThis.Math.pow(ToneMap.tmoCandelaLambert(rwl), this.lrwExponent) * this.lrwmDisplay - this.invCMaximum);
      scale = m > 0.0 ? m / rwl : 0.0;
    }
    else {
      scale = 0.0;
    }

    radiance.scale(scale);
    return radiance;
  }
}
