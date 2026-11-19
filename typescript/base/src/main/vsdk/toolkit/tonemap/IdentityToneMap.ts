import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { ToneMap } from "./ToneMap";
import { ToneMappingContext } from "./ToneMappingContext";

export class IdentityToneMap extends ToneMap {
  public constructor() {
    super();
  }

  public override init(toneMapOptions: ToneMappingContext): void {
    void toneMapOptions;
  }

  public override scaleForComputations(radiance: ColorRgbMutable): ColorRgbMutable {
    return radiance;
  }

  public override scaleForDisplay(radiance: ColorRgbMutable): ColorRgbMutable {
    return radiance;
  }
}
