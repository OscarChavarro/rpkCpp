import { Cie } from "vitral/dist/vsdk/toolkit/common/color/Cie";
import { ColorRgbMutable } from "../common/color/ColorRgbMutable";
import { ToneMap } from "./ToneMap";
import { ToneMapAdaptationMethod } from "./ToneMapAdaptationMethod";

export class ToneMappingContext {
  public static readonly GAMMA_TABLE_BITS = 12;
  public static readonly GAMMA_TABLE_SIZE = (1 << ToneMappingContext.GAMMA_TABLE_BITS) + 1;

  public brightness_adjust: number;
  public pow_bright_adjust: number;

  public staticAdaptationMethod: ToneMapAdaptationMethod;
  public realWorldAdaptionLuminance: number;
  public maximumDisplayLuminance: number;
  public maximumDisplayContrast: number;

  public gamma: ColorRgbMutable;
  public gammaTab: number[][];

  // Conversion from radiance (COLOR type) to display RGB: monitor primary
  // colors and white point
  private colorSpace: Cie;

  private static readonly DEFAULT_GAMMA = 1.7;
  private static readonly DEFAULT_TM_LWA = 10.0;
  private static readonly DEFAULT_TM_LD_MAXIMUM = 100.0;
  private static readonly DEFAULT_TM_C_MAXIMUM = 50.0;

  public constructor() {
    this.brightness_adjust = 0.0;
    this.pow_bright_adjust = globalThis.Math.pow(2.0, this.brightness_adjust);

    this.staticAdaptationMethod = ToneMapAdaptationMethod.TMA_MEDIAN;
    this.realWorldAdaptionLuminance = ToneMappingContext.DEFAULT_TM_LWA;
    this.maximumDisplayLuminance = ToneMappingContext.DEFAULT_TM_LD_MAXIMUM;
    this.maximumDisplayContrast = ToneMappingContext.DEFAULT_TM_C_MAXIMUM;

    // Monitor primaries and white point
    this.colorSpace = new Cie(
      0.640, 0.330,
      0.290, 0.600,
      0.150, 0.060,
      0.333333333333, 0.333333333333);

    this.gamma = new ColorRgbMutable();
    this.gamma.set(ToneMappingContext.DEFAULT_GAMMA, ToneMappingContext.DEFAULT_GAMMA, ToneMappingContext.DEFAULT_GAMMA);
    this.gammaTab = [
      new Array<number>(ToneMappingContext.GAMMA_TABLE_SIZE).fill(0.0),
      new Array<number>(ToneMappingContext.GAMMA_TABLE_SIZE).fill(0.0),
      new Array<number>(ToneMappingContext.GAMMA_TABLE_SIZE).fill(0.0)
    ];
    ToneMap.recomputeGammaTables(this, this.gamma);
  }

  public getColorSpace(): Cie {
    return this.colorSpace;
  }

  public setColorSpace(inColorSpace: Cie): void {
    this.colorSpace = inColorSpace;
  }
}
