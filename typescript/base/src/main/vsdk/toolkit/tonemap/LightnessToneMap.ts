import { Cie } from "../common/color/Cie";
import { ColorRgb } from "../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../common/logging/Logger";
import { Statistics } from "../common/statistics/Statistics";
import { ToneMap } from "./ToneMap";
import { ToneMappingContext } from "./ToneMappingContext";

export class LightnessToneMap extends ToneMap {
  private static lightness(luminance: number): number {
    if (Statistics.instance().radiance.referenceLuminance === 0.0) {
      return 0.0;
    }

    const relativeLuminance = luminance / Statistics.instance().radiance.referenceLuminance;
    if (relativeLuminance > 0.008856) {
      return 1.16 * globalThis.Math.pow(relativeLuminance, 0.33) - 0.16;
    }
    return 9.033 * relativeLuminance;
  }

  public constructor() {
    super();
  }

  public override init(toneMapOptions: ToneMappingContext): void {
    void toneMapOptions;
  }

  public override scaleForComputations(radiance: ColorRgb): ColorRgb {
    VsdkLogger.warning("ScaleForComputations", "%s %d not yet implemented", "LightnessToneMap.cpp", 0);
    return radiance;
  }

  public override scaleForDisplay(radiance: ColorRgb): ColorRgb {
    const max = radiance.maximumComponent();
    if (max < 1e-32) {
      return radiance;
    }

    const scaleFactor = LightnessToneMap.lightness(Cie.WHITE_EFFICACY * max);
    if (scaleFactor === 0.0) {
      return radiance;
    }

    radiance.scale(scaleFactor / max);
    return radiance;
  }
}
