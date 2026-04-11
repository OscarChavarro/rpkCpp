import { Cie } from "../common/Cie";
import { ColorRgb } from "../common/ColorRgb";
import { ToneMap } from "./ToneMap";
import { ToneMappingContext } from "./ToneMappingContext";

export class WardToneMap extends ToneMap {
  private comp: number;
  private display: number;
  private lda: number;

  public constructor() {
    super();
    this.comp = 0.0;
    this.display = 0.0;
    this.lda = 0.0;
  }

  public override init(toneMapOptions: ToneMappingContext): void {
    const realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
    const maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    this.lda = maximumDisplayLuminance / 2.0;

    const p1 = globalThis.Math.pow(this.lda, 0.4);
    const p2 = globalThis.Math.pow(realWorldAdaptionLuminance, 0.4);
    const p3 = (1.219 + p1) / (1.219 + p2);
    this.comp = globalThis.Math.pow(p3, 2.5);
    this.display = this.comp / maximumDisplayLuminance;
  }

  public override scaleForComputations(radiance: ColorRgb): ColorRgb {
    radiance.scale(this.comp);
    return radiance;
  }

  public override scaleForDisplay(radiance: ColorRgb): ColorRgb {
    const eff = Cie.getLuminousEfficacy();
    radiance.scale(eff * this.display);
    return radiance;
  }
}
