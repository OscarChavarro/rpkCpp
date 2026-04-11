import { Cie } from "../common/Cie";
import { ColorRgb } from "../common/ColorRgb";
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

  public xr: number;
  public yr: number;
  public xg: number;
  public yg: number;
  public xb: number;
  public yb: number;
  public xw: number;
  public yw: number;

  public gamma: ColorRgb;
  public gammaTab: number[][];

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

    this.xr = 0.640;
    this.yr = 0.330;
    this.xg = 0.290;
    this.yg = 0.600;
    this.xb = 0.150;
    this.yb = 0.060;
    this.xw = 0.333333333333;
    this.yw = 0.333333333333;
    Cie.computeColorConversionTransforms(this.xr, this.yr, this.xg, this.yg, this.xb, this.yb, this.xw, this.yw);

    this.gamma = new ColorRgb();
    this.gamma.set(ToneMappingContext.DEFAULT_GAMMA, ToneMappingContext.DEFAULT_GAMMA, ToneMappingContext.DEFAULT_GAMMA);
    this.gammaTab = [
      new Array<number>(ToneMappingContext.GAMMA_TABLE_SIZE).fill(0.0),
      new Array<number>(ToneMappingContext.GAMMA_TABLE_SIZE).fill(0.0),
      new Array<number>(ToneMappingContext.GAMMA_TABLE_SIZE).fill(0.0)
    ];
    ToneMap.recomputeGammaTables(this, this.gamma);
  }
}
