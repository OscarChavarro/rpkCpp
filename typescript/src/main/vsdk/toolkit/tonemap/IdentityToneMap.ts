import { ColorRgb } from "../common/color/ColorRgb";
import { ToneMap } from "./ToneMap";
import { ToneMappingContext } from "./ToneMappingContext";

export class IdentityToneMap extends ToneMap {
  public constructor() {
    super();
  }

  public override init(toneMapOptions: ToneMappingContext): void {
    void toneMapOptions;
  }

  public override scaleForComputations(radiance: ColorRgb): ColorRgb {
    return radiance;
  }

  public override scaleForDisplay(radiance: ColorRgb): ColorRgb {
    return radiance;
  }
}
