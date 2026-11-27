import { String as JavaString } from "vitral/dist/java/lang/String";
import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
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

  protected override initParameters(toneMapOptions: ToneMappingContext): void {
    void toneMapOptions;
  }

  public override scaleForComputations(radiance: ColorRgbMutable): ColorRgbMutable {
    VsdkLogger.reportMessage(this, VsdkLogger.WARNING, "ScaleForComputations", JavaString.vformat("%s %d not yet implemented", ["LightnessToneMap.cpp", 0]));
    return radiance;
  }

  public override scaleForDisplay(radiance: ColorRgbMutable): ColorRgbMutable {
    const max = radiance.maximumComponent();
    if (max < 1e-32) {
      return radiance;
    }

    const scaleFactor = LightnessToneMap.lightness(this.getColorSpace().getLuminousEfficacy() * max);
    if (scaleFactor === 0.0) {
      return radiance;
    }

    radiance.scale(scaleFactor / max);
    return radiance;
  }
}
