import { Cie } from "../../common/Cie";
import { ColorRgb } from "../../common/ColorRgb";
import { Error as VsdkError } from "../../common/Error";
import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { ToneMap } from "../../tonemap/ToneMap";
import { ToneMapAdaptationMethod } from "../../tonemap/ToneMapAdaptationMethod";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { OptionTextUtils } from "./OptionTextUtils";

export class OptionsGroupToneMapping {
  private static readonly TONE_MAPPING_METHODS_STRING_LENGTH = 1000;

  private static toneMappingMethodsString = "";
  private static toneMapName: string[] | null = null;
  private static toneMapOptions: ToneMappingContext | null = null;

  private constructor() {
  }

  private static makeToneMappingMethodsString(): void {
    OptionsGroupToneMapping.toneMappingMethodsString =
      "-tonemapping <method>: Set tone mapping method\n"
      + "\tmethods: Lightness            Lightness Mapping (default)\n"
      + "\t         TumblinRushmeier     Tumblin/Rushmeier's Mapping\n"
      + "\t         Ward                 Ward's Mapping\n"
      + "\t         RevisedTR            Revised Tumblin/Rushmeier's Mapping\n"
      + "\t         Ferwerda             Partial Ferwerda's Mapping";

    if (OptionsGroupToneMapping.toneMappingMethodsString.length > OptionsGroupToneMapping.TONE_MAPPING_METHODS_STRING_LENGTH) {
      OptionsGroupToneMapping.toneMappingMethodsString =
        OptionsGroupToneMapping.toneMappingMethodsString.substring(0, OptionsGroupToneMapping.TONE_MAPPING_METHODS_STRING_LENGTH);
    }
  }

  private static toneMappingMethodOption(name: TypedOption.MutableValue<string | null>): void {
    if (OptionsGroupToneMapping.toneMapName !== null && OptionsGroupToneMapping.toneMapName.length > 0) {
      OptionsGroupToneMapping.toneMapName[0] = name.value ?? "";
    }
  }

  private static brightnessAdjustOption(_value: TypedOption.MutableValue<number>): void {
    if (OptionsGroupToneMapping.toneMapOptions === null) {
      VsdkError.fatal(-1, "CommandLineToneMappingOptionsGroup::brightnessAdjustOption", "ToneMappingContext not set");
      return;
    }
    OptionsGroupToneMapping.toneMapOptions.pow_bright_adjust = globalThis.Math.pow(
      2.0,
      OptionsGroupToneMapping.toneMapOptions.brightness_adjust
    );
  }

  private static redChromaOption(value: TypedOption.MutableValue<Vector3D>): void {
    if (OptionsGroupToneMapping.toneMapOptions === null) {
      VsdkError.fatal(-1, "CommandLineToneMappingOptionsGroup::redChromaOption", "ToneMappingContext not set");
      return;
    }
    OptionsGroupToneMapping.toneMapOptions.xr = value.value.x;
    OptionsGroupToneMapping.toneMapOptions.yr = value.value.y;
    Cie.computeColorConversionTransforms(
      OptionsGroupToneMapping.toneMapOptions.xr,
      OptionsGroupToneMapping.toneMapOptions.yr,
      OptionsGroupToneMapping.toneMapOptions.xg,
      OptionsGroupToneMapping.toneMapOptions.yg,
      OptionsGroupToneMapping.toneMapOptions.xb,
      OptionsGroupToneMapping.toneMapOptions.yb,
      OptionsGroupToneMapping.toneMapOptions.xw,
      OptionsGroupToneMapping.toneMapOptions.yw
    );
  }

  private static greenChromaOption(value: TypedOption.MutableValue<Vector3D>): void {
    if (OptionsGroupToneMapping.toneMapOptions === null) {
      VsdkError.fatal(-1, "CommandLineToneMappingOptionsGroup::greenChromaOption", "ToneMappingContext not set");
      return;
    }
    OptionsGroupToneMapping.toneMapOptions.xg = value.value.x;
    OptionsGroupToneMapping.toneMapOptions.yg = value.value.y;
    Cie.computeColorConversionTransforms(
      OptionsGroupToneMapping.toneMapOptions.xr,
      OptionsGroupToneMapping.toneMapOptions.yr,
      OptionsGroupToneMapping.toneMapOptions.xg,
      OptionsGroupToneMapping.toneMapOptions.yg,
      OptionsGroupToneMapping.toneMapOptions.xb,
      OptionsGroupToneMapping.toneMapOptions.yb,
      OptionsGroupToneMapping.toneMapOptions.xw,
      OptionsGroupToneMapping.toneMapOptions.yw
    );
  }

  private static blueChromaOption(value: TypedOption.MutableValue<Vector3D>): void {
    if (OptionsGroupToneMapping.toneMapOptions === null) {
      VsdkError.fatal(-1, "CommandLineToneMappingOptionsGroup::blueChromaOption", "ToneMappingContext not set");
      return;
    }
    OptionsGroupToneMapping.toneMapOptions.xb = value.value.x;
    OptionsGroupToneMapping.toneMapOptions.yb = value.value.y;
    Cie.computeColorConversionTransforms(
      OptionsGroupToneMapping.toneMapOptions.xr,
      OptionsGroupToneMapping.toneMapOptions.yr,
      OptionsGroupToneMapping.toneMapOptions.xg,
      OptionsGroupToneMapping.toneMapOptions.yg,
      OptionsGroupToneMapping.toneMapOptions.xb,
      OptionsGroupToneMapping.toneMapOptions.yb,
      OptionsGroupToneMapping.toneMapOptions.xw,
      OptionsGroupToneMapping.toneMapOptions.yw
    );
  }

  private static whiteChromaOption(value: TypedOption.MutableValue<Vector3D>): void {
    if (OptionsGroupToneMapping.toneMapOptions === null) {
      VsdkError.fatal(-1, "CommandLineToneMappingOptionsGroup::whiteChromaOption", "ToneMappingContext not set");
      return;
    }
    OptionsGroupToneMapping.toneMapOptions.xw = value.value.x;
    OptionsGroupToneMapping.toneMapOptions.yw = value.value.y;
    Cie.computeColorConversionTransforms(
      OptionsGroupToneMapping.toneMapOptions.xr,
      OptionsGroupToneMapping.toneMapOptions.yr,
      OptionsGroupToneMapping.toneMapOptions.xg,
      OptionsGroupToneMapping.toneMapOptions.yg,
      OptionsGroupToneMapping.toneMapOptions.xb,
      OptionsGroupToneMapping.toneMapOptions.yb,
      OptionsGroupToneMapping.toneMapOptions.xw,
      OptionsGroupToneMapping.toneMapOptions.yw
    );
  }

  private static toneMappingCommandLineOptionDescAdaptMethodOption(name: TypedOption.MutableValue<string | null>): void {
    if (OptionsGroupToneMapping.toneMapOptions === null) {
      VsdkError.fatal(
        -1,
        "CommandLineToneMappingOptionsGroup::toneMappingCommandLineOptionDescAdaptMethodOption",
        "ToneMappingContext not set"
      );
      return;
    }
    const value = name.value ?? "";
    if (OptionTextUtils.equalsIgnoreCasePrefix(value, "average", 2)) {
      OptionsGroupToneMapping.toneMapOptions.staticAdaptationMethod = ToneMapAdaptationMethod.TMA_AVERAGE;
    }
    else if (OptionTextUtils.equalsIgnoreCasePrefix(value, "median", 2)) {
      OptionsGroupToneMapping.toneMapOptions.staticAdaptationMethod = ToneMapAdaptationMethod.TMA_MEDIAN;
    }
    else {
      VsdkError.error(null, "Invalid adaptation estimate method '%s'", value);
    }
  }

  private static gammaOption(gam: TypedOption.MutableValue<number>): void {
    if (OptionsGroupToneMapping.toneMapOptions === null) {
      VsdkError.fatal(-1, "CommandLineToneMappingOptionsGroup::gammaOption", "ToneMappingContext not set");
      return;
    }
    OptionsGroupToneMapping.toneMapOptions.gamma.set(gam.value, gam.value, gam.value);
  }

  public static toneMapParseOptions(
    argc: number[],
    argv: string[],
    toneMapNameOut: string[],
    toneMapOptionsContext: ToneMappingContext
  ): void {
    const toneMapMethodName = TypedOption.valueRef<string | null>(null);
    const adaptMethodName = TypedOption.valueRef<string | null>(null);
    const redChromaticityValue = new Vector3D(0.0, 0.0, 0.0);
    const greenChromaticityValue = new Vector3D(0.0, 0.0, 0.0);
    const blueChromaticityValue = new Vector3D(0.0, 0.0, 0.0);
    const whiteChromaticityValue = new Vector3D(0.0, 0.0, 0.0);

    const toneMappingOpt = new TypedOption<string | null>(
      "-tonemapping",
      toneMapMethodName,
      1,
      OptionsGroupToneMapping.toneMappingMethodOption,
      null
    );
    const brightnessAdjustOpt = new TypedOption<number>(
      "-brightness-adjust",
      TypedOption.reference(
        () => toneMapOptionsContext.brightness_adjust,
        (v) => {
          toneMapOptionsContext.brightness_adjust = v;
        }
      ),
      1,
      OptionsGroupToneMapping.brightnessAdjustOption,
      null
    );
    const adaptOpt = new TypedOption<string | null>(
      "-adapt",
      adaptMethodName,
      1,
      OptionsGroupToneMapping.toneMappingCommandLineOptionDescAdaptMethodOption,
      null
    );
    const lwaOpt = new TypedOption<number>(
      "-lwa",
      TypedOption.reference(
        () => toneMapOptionsContext.realWorldAdaptionLuminance,
        (v) => {
          toneMapOptionsContext.realWorldAdaptionLuminance = v;
        }
      ),
      1,
      null,
      null
    );
    const ldmaxOpt = new TypedOption<number>(
      "-ldmax",
      TypedOption.reference(
        () => toneMapOptionsContext.maximumDisplayLuminance,
        (v) => {
          toneMapOptionsContext.maximumDisplayLuminance = v;
        }
      ),
      1,
      null,
      null
    );
    const cmaxOpt = new TypedOption<number>(
      "-cmax",
      TypedOption.reference(
        () => toneMapOptionsContext.maximumDisplayContrast,
        (v) => {
          toneMapOptionsContext.maximumDisplayContrast = v;
        }
      ),
      1,
      null,
      null
    );
    const gammaOpt = new TypedOption<number>(
      "-gamma",
      TypedOption.reference(
        () => toneMapOptionsContext.gamma.r,
        (v) => {
          toneMapOptionsContext.gamma.r = v;
        }
      ),
      1,
      OptionsGroupToneMapping.gammaOption,
      null
    );
    const rgbGammaOpt = new TypedOption<ColorRgb>(
      "-rgbgamma",
      TypedOption.reference(
        () => toneMapOptionsContext.gamma,
        (v) => {
          toneMapOptionsContext.gamma = v;
        }
      ),
      3,
      null,
      OptionsGroupToneMapping.parseColor3
    );
    const redOpt = new TypedOption<Vector3D>(
      "-red",
      TypedOption.valueRef(redChromaticityValue),
      2,
      OptionsGroupToneMapping.redChromaOption,
      OptionsGroupToneMapping.parseCieXy
    );
    const greenOpt = new TypedOption<Vector3D>(
      "-green",
      TypedOption.valueRef(greenChromaticityValue),
      2,
      OptionsGroupToneMapping.greenChromaOption,
      OptionsGroupToneMapping.parseCieXy
    );
    const blueOpt = new TypedOption<Vector3D>(
      "-blue",
      TypedOption.valueRef(blueChromaticityValue),
      2,
      OptionsGroupToneMapping.blueChromaOption,
      OptionsGroupToneMapping.parseCieXy
    );
    const whiteOpt = new TypedOption<Vector3D>(
      "-white",
      TypedOption.valueRef(whiteChromaticityValue),
      2,
      OptionsGroupToneMapping.whiteChromaOption,
      OptionsGroupToneMapping.parseCieXy
    );
    const toneMappingOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(toneMappingOpt, 4),
      TypedOption.REGISTER_OPTION(brightnessAdjustOpt, 4),
      TypedOption.REGISTER_OPTION(adaptOpt, 5),
      TypedOption.REGISTER_OPTION(lwaOpt, 3),
      TypedOption.REGISTER_OPTION(ldmaxOpt, 5),
      TypedOption.REGISTER_OPTION(cmaxOpt, 4),
      TypedOption.REGISTER_OPTION(gammaOpt, 4),
      TypedOption.REGISTER_OPTION(rgbGammaOpt, 4),
      TypedOption.REGISTER_OPTION(redOpt, 4),
      TypedOption.REGISTER_OPTION(greenOpt, 4),
      TypedOption.REGISTER_OPTION(blueOpt, 4),
      TypedOption.REGISTER_OPTION(whiteOpt, 4),
    ];

    OptionsGroupToneMapping.toneMapName = toneMapNameOut;
    OptionsGroupToneMapping.toneMapOptions = toneMapOptionsContext;
    OptionsGroupToneMapping.makeToneMappingMethodsString();
    const toneMappingGroups = [
      new OptionGroup("toneMapping", toneMappingOptions, 12),
    ];
    OptionParser.parse(argc, argv, toneMappingGroups, 1);
    ToneMap.recomputeGammaTables(toneMapOptionsContext, (OptionsGroupToneMapping.toneMapOptions as ToneMappingContext).gamma);
    OptionsGroupToneMapping.toneMapOptions = null;
    OptionsGroupToneMapping.toneMapName = null;
  }

  private static parseColor3(
    argc: number,
    argv: string[] | null,
    value: TypedOption.MutableValue<ColorRgb>
  ): boolean {
    if (argc < 3 || argv === null || argv[0] === null || argv[1] === null || argv[2] === null) {
      return false;
    }
    const r = Number.parseFloat(argv[0]);
    const g = Number.parseFloat(argv[1]);
    const b = Number.parseFloat(argv[2]);
    if (Number.isNaN(r) || Number.isNaN(g) || Number.isNaN(b)) {
      return false;
    }
    value.value.r = r;
    value.value.g = g;
    value.value.b = b;
    return true;
  }

  private static parseCieXy(
    argc: number,
    argv: string[] | null,
    value: TypedOption.MutableValue<Vector3D>
  ): boolean {
    if (argc < 2 || argv === null || argv[0] === null || argv[1] === null) {
      return false;
    }
    const x = Number.parseFloat(argv[0]);
    const y = Number.parseFloat(argv[1]);
    if (Number.isNaN(x) || Number.isNaN(y)) {
      return false;
    }
    value.value.x = x;
    value.value.y = y;
    value.value.z = 0.0;
    return true;
  }
}
