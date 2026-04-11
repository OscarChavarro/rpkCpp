import { Cie } from "../common/Cie";
import { ColorRgb } from "../common/ColorRgb";
import { ToneMap } from "./ToneMap";
import { ToneMappingContext } from "./ToneMappingContext";

export class FerwerdaToneMap extends ToneMap {
  private sf: ColorRgb;
  private msf: number;
  private pmComp: number;
  private pmDisplay: number;
  private smComp: number;
  private smDisplay: number;
  private lda: number;

  private static photopicOperator(logLa: number): number {
    let r: number;
    if (logLa <= -2.6) {
      r = -0.72;
    }
    else if (logLa >= 1.9) {
      r = logLa - 1.255;
    }
    else {
      r = globalThis.Math.pow(0.249 * logLa + 0.65, 2.7) - 0.72;
    }

    return globalThis.Math.pow(10.0, r);
  }

  private static scotopicOperator(logLa: number): number {
    let r: number;
    if (logLa <= -3.94) {
      r = -2.86;
    }
    else if (logLa >= -1.44) {
      r = logLa - 0.395;
    }
    else {
      r = globalThis.Math.pow(0.405 * logLa + 1.6, 2.18) - 2.86;
    }

    return globalThis.Math.pow(10.0, r);
  }

  private static mesopicScaleFactor(logLwa: number): number {
    if (logLwa < -2.5) {
      return 1.0;
    }
    if (logLwa > 0.8) {
      return 0.0;
    }
    return (0.8 - logLwa) / 3.3;
  }

  public constructor() {
    super();
    this.sf = new ColorRgb(0.062, 0.608, 0.330);
    this.msf = 0.0;
    this.pmComp = 0.0;
    this.pmDisplay = 0.0;
    this.smComp = 0.0;
    this.smDisplay = 0.0;
    this.lda = 0.0;
  }

  public override init(toneMapOptions: ToneMappingContext): void {
    const realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
    const maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    this.lda = maximumDisplayLuminance / 2.0;

    this.msf = FerwerdaToneMap.mesopicScaleFactor(globalThis.Math.log10(realWorldAdaptionLuminance));
    this.smComp = FerwerdaToneMap.scotopicOperator(globalThis.Math.log10(this.lda)) /
      FerwerdaToneMap.scotopicOperator(globalThis.Math.log10(realWorldAdaptionLuminance));
    this.pmComp = FerwerdaToneMap.photopicOperator(globalThis.Math.log10(this.lda)) /
      FerwerdaToneMap.photopicOperator(globalThis.Math.log10(realWorldAdaptionLuminance));
    this.smDisplay = this.smComp / maximumDisplayLuminance;
    this.pmDisplay = this.pmComp / maximumDisplayLuminance;
  }

  public override scaleForComputations(radiance: ColorRgb): ColorRgb {
    const p = new ColorRgb();
    let sl: number;

    const eff = Cie.getLuminousEfficacy();
    radiance.scale(eff);

    p.set(radiance.r, radiance.g, radiance.b);
    sl = this.smComp * this.msf * (p.r * this.sf.r + p.g * this.sf.g + p.b * this.sf.b);

    radiance.scale(this.pmComp);

    if (sl > 0.0) {
      radiance.addConstant(radiance, sl);
    }

    return radiance;
  }

  public override scaleForDisplay(radiance: ColorRgb): ColorRgb {
    const p = new ColorRgb();
    let sl: number;

    const eff = Cie.getLuminousEfficacy();
    radiance.scale(eff);

    radiance.set(p.r, p.g, p.b);
    sl = this.smDisplay * this.msf * (p.r * this.sf.r + p.g * this.sf.g + p.b * this.sf.b);

    radiance.scale(this.pmDisplay);

    if (sl > 0.0) {
      radiance.addConstant(radiance, sl);
    }

    return radiance;
  }
}
