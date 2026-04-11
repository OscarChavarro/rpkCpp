import { ColorRgb } from "../../common/ColorRgb";
import { RenderOptions } from "../../common/RenderOptions";
import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";
import { ParseRuntimeContext } from "../../io/context/ParseRuntimeContext";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { ConstantColorBackground } from "../../scene/ConstantColorBackground";
import { Scene } from "../../scene/Scene";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { EnumAppOptions } from "./EnumAppOptions";
import { EnumBackgroundMode } from "./EnumBackgroundMode";
import { OptionTextUtils } from "./OptionTextUtils";
import { OptionsGroupCamera } from "./OptionsGroupCamera";
import { OptionsGroupRender } from "./OptionsGroupRender";
import { OptionsGroupToneMapping } from "./OptionsGroupToneMapping";

export class OptionsGroupCore {
  private static readonly DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS = 4;
  private static readonly DEFAULT_FORCE_ONE_SIDED = true;
  private static readonly DEFAULT_BACKGROUND_COLOR = new ColorRgb(0.0, 0.0, 0.0);

  private static numberOfQuarterCircleDivisions = OptionsGroupCore.DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
  private static fileOptionsForceOneSidedSurfaces = 0;
  private static outputImageWidth = 1920;
  private static outputImageHeight = 1080;
  private static glutDebugEnabled = 0;
  private static backgroundMode = EnumBackgroundMode.NONE;
  private static backgroundColor = new ColorRgb(
    OptionsGroupCore.DEFAULT_BACKGROUND_COLOR.r,
    OptionsGroupCore.DEFAULT_BACKGROUND_COLOR.g,
    OptionsGroupCore.DEFAULT_BACKGROUND_COLOR.b
  );

  private constructor() {
  }

  public static parse(
    argc: number[],
    argv: string[],
    parseSession: ParseRuntimeContext,
    scene: Scene,
    renderOptions: RenderOptions,
    toneMapOptions: ToneMappingContext,
    imageOutputWidth: number[],
    imageOutputHeight: number[],
    glutDebugEnabledOut: boolean[],
    toneMapNameOut: string[]
  ): void {
    const oneSidedSurfaces = [parseSession.singleSided];
    const conicSubDivisions = [parseSession.numberOfQuarterCircleDivisions];

    OptionsGroupCore.commandLineGeneralProgramParseOptions(
      argc,
      argv,
      oneSidedSurfaces,
      conicSubDivisions,
      imageOutputWidth,
      imageOutputHeight,
      glutDebugEnabledOut
    );

    parseSession.singleSided = oneSidedSurfaces[0];
    parseSession.parserConfig.singleSided = oneSidedSurfaces[0];
    parseSession.numberOfQuarterCircleDivisions = conicSubDivisions[0];
    parseSession.parserConfig.numberOfQuarterCircleDivisions = conicSubDivisions[0];

    OptionsGroupRender.renderParseOptions(argc, argv, renderOptions);
    OptionsGroupToneMapping.toneMapParseOptions(argc, argv, toneMapNameOut, toneMapOptions);

    if (scene.camera === null) {
      scene.camera = new Camera();
    }
    OptionsGroupCamera.cameraParseOptions(
      argc,
      argv,
      scene.camera,
      imageOutputWidth[0],
      imageOutputHeight[0]
    );
  }

  public static createBackground(): Background | null {
    return OptionsGroupCore.commandLineCreateBackground();
  }

  public static commandLineCreateBackground(): Background | null {
    if (OptionsGroupCore.backgroundMode === EnumBackgroundMode.SOLID) {
      return new ConstantColorBackground(OptionsGroupCore.backgroundColor);
    }
    return null;
  }

  private static commandLineParseFloat(text: string | null, value: number[]): boolean {
    if (text === null || value === null || value.length === 0) {
      return false;
    }

    const parsed = Number.parseFloat(text);
    if (Number.isNaN(parsed)) {
      return false;
    }
    value[0] = parsed;
    return true;
  }

  private static commandLineParseBackgroundColor(rArg: string | null, gArg: string | null, bArg: string | null, color: ColorRgb): boolean {
    const red = [0.0];
    const green = [0.0];
    const blue = [0.0];

    if (
      !OptionsGroupCore.commandLineParseFloat(rArg, red)
      || !OptionsGroupCore.commandLineParseFloat(gArg, green)
      || !OptionsGroupCore.commandLineParseFloat(bArg, blue)
    ) {
      return false;
    }

    if (
      red[0] < 0.0 || red[0] > 1.0
      || green[0] < 0.0 || green[0] > 1.0
      || blue[0] < 0.0 || blue[0] > 1.0
    ) {
      return false;
    }

    color.set(red[0], green[0], blue[0]);
    return true;
  }

  private static commandLineParseBackgroundOption(argc: number[], argv: string[]): void {
    if (argc === null || argc.length === 0 || argv === null) {
      return;
    }

    const args = argv as Array<string | null>;
    let writeIndex = 0;
    let readIndex = 0;
    while (readIndex < argc[0]) {
      const argument = args[readIndex];
      if (argument === null || argument !== "-background") {
        args[writeIndex++] = args[readIndex++];
        continue;
      }

      if (readIndex + 1 >= argc[0]) {
        process.stderr.write("Option '-background' requires a mode. Supported mode: solid.\n");
        readIndex += 1;
        continue;
      }

      const mode = args[readIndex + 1];
      if (!OptionTextUtils.equalsIgnoreCase(mode, "solid")) {
        process.stderr.write(
          `Invalid background mode '${mode}'. Expected '-background solid <r> <g> <b>'.\n`
        );
        readIndex += 2;
        continue;
      }

      if (readIndex + 4 >= argc[0]) {
        process.stderr.write(
          "Option '-background solid' requires three values in range [0.0, 1.0].\n"
        );
        readIndex += 2;
        continue;
      }

      const parsedColor = new ColorRgb();
      if (
        !OptionsGroupCore.commandLineParseBackgroundColor(
          args[readIndex + 2],
          args[readIndex + 3],
          args[readIndex + 4],
          parsedColor
        )
      ) {
        process.stderr.write(
          "Invalid '-background solid' color. Use '-background solid <r> <g> <b>' with values in [0.0, 1.0].\n"
        );
      }
      else {
        OptionsGroupCore.backgroundMode = EnumBackgroundMode.SOLID;
        OptionsGroupCore.backgroundColor = parsedColor;
      }
      readIndex += 5;
    }

    while (writeIndex < argc[0]) {
      args[writeIndex++] = null;
    }
    argc[0] = writeIndex;
  }

  private static mainForceOneSidedOption(value: TypedOption.MutableValue<number>): void {
    OptionsGroupCore.fileOptionsForceOneSidedSurfaces = value.value;
  }

  private static mainMonochromeOption(value: TypedOption.MutableValue<number>): void {
    OptionsGroupCore.numberOfQuarterCircleDivisions = value.value;
  }

  private static setIntTrue(value: TypedOption.MutableValue<number>): void {
    value.value = 1;
  }

  public static commandLineGeneralProgramParseOptions(
    argc: number[],
    argv: string[],
    oneSidedSurfaces: boolean[],
    conicSubDivisions: number[],
    imageOutputWidth: number[],
    imageOutputHeight: number[],
    glutDebugEnabledOut: boolean[]
  ): void {
    const appOptions = new EnumAppOptions();
    appOptions.width = OptionsGroupCore.outputImageWidth;
    appOptions.height = OptionsGroupCore.outputImageHeight;
    appOptions.nqcdivs = OptionsGroupCore.numberOfQuarterCircleDivisions;
    appOptions.yesValue = 1;
    appOptions.noValue = 0;
    appOptions.debug = 0;

    const widthOpt = new TypedOption<number>(
      "-width",
      TypedOption.reference(() => appOptions.width, (v) => {
        appOptions.width = v;
      }),
      1,
      null,
      null
    );
    const heightOpt = new TypedOption<number>(
      "-height",
      TypedOption.reference(() => appOptions.height, (v) => {
        appOptions.height = v;
      }),
      1,
      null,
      null
    );
    const nqcdivsOpt = new TypedOption<number>(
      "-nqcdivs",
      TypedOption.reference(() => appOptions.nqcdivs, (v) => {
        appOptions.nqcdivs = v;
      }),
      1,
      null,
      null
    );
    const forceOneSidedOpt = new TypedOption<number>(
      "-force-onesided",
      TypedOption.reference(() => appOptions.yesValue, (v) => {
        appOptions.yesValue = v;
      }),
      0,
      OptionsGroupCore.mainForceOneSidedOption,
      null
    );
    const dontForceOneSidedOpt = new TypedOption<number>(
      "-dont-force-onesided",
      TypedOption.reference(() => appOptions.noValue, (v) => {
        appOptions.noValue = v;
      }),
      0,
      OptionsGroupCore.mainForceOneSidedOption,
      null
    );
    const monochromaticOpt = new TypedOption<number>(
      "-monochromatic",
      TypedOption.reference(() => appOptions.yesValue, (v) => {
        appOptions.yesValue = v;
      }),
      0,
      OptionsGroupCore.mainMonochromeOption,
      null
    );
    const glutDebugOpt = new TypedOption<number>(
      "-glutDebug",
      TypedOption.reference(() => appOptions.debug, (v) => {
        appOptions.debug = v;
      }),
      0,
      OptionsGroupCore.setIntTrue,
      null
    );
    const registry: OptionBase[] = [
      TypedOption.REGISTER_OPTION(widthOpt, 5),
      TypedOption.REGISTER_OPTION(heightOpt, 6),
      TypedOption.REGISTER_OPTION(nqcdivsOpt, 3),
      TypedOption.REGISTER_OPTION(forceOneSidedOpt, 10),
      TypedOption.REGISTER_OPTION(dontForceOneSidedOpt, 14),
      TypedOption.REGISTER_OPTION(monochromaticOpt, 5),
      TypedOption.REGISTER_OPTION(glutDebugOpt, 6),
    ];

    OptionsGroupCore.fileOptionsForceOneSidedSurfaces = OptionsGroupCore.DEFAULT_FORCE_ONE_SIDED ? 1 : 0;
    OptionsGroupCore.numberOfQuarterCircleDivisions = OptionsGroupCore.DEFAULT_NUMBER_OF_QUARTIC_DIVISIONS;
    OptionsGroupCore.backgroundMode = EnumBackgroundMode.NONE;
    OptionsGroupCore.backgroundColor = new ColorRgb(
      OptionsGroupCore.DEFAULT_BACKGROUND_COLOR.r,
      OptionsGroupCore.DEFAULT_BACKGROUND_COLOR.g,
      OptionsGroupCore.DEFAULT_BACKGROUND_COLOR.b
    );
    OptionsGroupCore.glutDebugEnabled = appOptions.debug;

    OptionsGroupCore.commandLineParseBackgroundOption(argc, argv);

    const generalGroups = [
      new OptionGroup("global", registry, 7),
    ];
    OptionParser.parse(argc, argv, generalGroups, 1);

    OptionsGroupCore.outputImageWidth = appOptions.width;
    OptionsGroupCore.outputImageHeight = appOptions.height;
    OptionsGroupCore.numberOfQuarterCircleDivisions = appOptions.nqcdivs;
    OptionsGroupCore.glutDebugEnabled = appOptions.debug;

    if (oneSidedSurfaces !== null && oneSidedSurfaces.length > 0) {
      oneSidedSurfaces[0] = OptionsGroupCore.fileOptionsForceOneSidedSurfaces !== 0;
    }
    if (conicSubDivisions !== null && conicSubDivisions.length > 0) {
      conicSubDivisions[0] = OptionsGroupCore.numberOfQuarterCircleDivisions;
    }
    if (imageOutputWidth !== null && imageOutputWidth.length > 0) {
      imageOutputWidth[0] = OptionsGroupCore.outputImageWidth;
    }
    if (imageOutputHeight !== null && imageOutputHeight.length > 0) {
      imageOutputHeight[0] = OptionsGroupCore.outputImageHeight;
    }
    if (glutDebugEnabledOut !== null && glutDebugEnabledOut.length > 0) {
      glutDebugEnabledOut[0] = OptionsGroupCore.glutDebugEnabled !== 0;
    }
  }
}
