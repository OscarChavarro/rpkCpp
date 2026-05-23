import { ColorRgb } from "../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../common/logging/Logger";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { ToneMappingContext } from "./ToneMappingContext";

export abstract class ToneMap {
  private static activeToneMap: ToneMap | null = null;

  private static gammaTableEntry(x: number): number {
    return globalThis.Math.trunc(x * (1 << ToneMappingContext.GAMMA_TABLE_BITS));
  }

  private static recomputeGammaTable(toneMapOptions: ToneMappingContext, index: number, gamma: number): void {
    if (gamma <= Numeric.EPSILON) {
      gamma = 1.0;
    }
    for (let i = 0; i <= (1 << ToneMappingContext.GAMMA_TABLE_BITS); i++) {
      toneMapOptions.gammaTab[index]![i] = globalThis.Math.pow(
        i / (1 << ToneMappingContext.GAMMA_TABLE_BITS),
        1.0 / gamma
      );
    }
  }

  private static toneMapScaleForDisplay(radiance: ColorRgb): ColorRgb {
    if (ToneMap.activeToneMap === null) {
      VsdkLogger.fatal(-1, "ToneMap::toneMapScaleForDisplay", "No active tone map");
    }
    return (ToneMap.activeToneMap as ToneMap).scaleForDisplay(radiance);
  }

  private static rescaleRadiance(input: ColorRgb, out: ColorRgb, toneMapOptions: ToneMappingContext): ColorRgb {
    const scaledInput = new ColorRgb(input.r, input.g, input.b);
    scaledInput.scale(toneMapOptions.pow_bright_adjust);
    const scaled = ToneMap.toneMapScaleForDisplay(scaledInput);
    out.set(scaled.r, scaled.g, scaled.b);
    return out;
  }

  public constructor() {
  }

  public abstract init(toneMapOptions: ToneMappingContext): void;

  protected static tmoCandelaLambert(a: number): number {
    return a * globalThis.Math.PI * 1e-4;
  }

  protected static tmoLambertCandela(a: number): number {
    return a / (globalThis.Math.PI * 1e-4);
  }

  public abstract scaleForComputations(radiance: ColorRgb): ColorRgb;

  public abstract scaleForDisplay(radiance: ColorRgb): ColorRgb;

  public static setActiveToneMap(toneMap: ToneMap): void {
    ToneMap.activeToneMap = toneMap;
  }

  public static toneMappingGammaCorrection(rgb: ColorRgb, toneMapOptions: ToneMappingContext): void {
    rgb.r = toneMapOptions.gammaTab[0]![ToneMap.gammaTableEntry(rgb.r)]!;
    rgb.g = toneMapOptions.gammaTab[1]![ToneMap.gammaTableEntry(rgb.g)]!;
    rgb.b = toneMapOptions.gammaTab[2]![ToneMap.gammaTableEntry(rgb.b)]!;
  }

  public static recomputeGammaTables(toneMapOptions: ToneMappingContext, gamma: ColorRgb): void {
    ToneMap.recomputeGammaTable(toneMapOptions, 0, gamma.r);
    ToneMap.recomputeGammaTable(toneMapOptions, 1, gamma.g);
    ToneMap.recomputeGammaTable(toneMapOptions, 2, gamma.b);
  }

  public static radianceToRgb(color: ColorRgb, rgb: ColorRgb, toneMapOptions: ToneMappingContext): ColorRgb {
    const rescaled = new ColorRgb();
    ToneMap.rescaleRadiance(color, rescaled, toneMapOptions);
    rgb.set(rescaled.r, rescaled.g, rescaled.b);
    rgb.clip();
    return rgb;
  }
}
