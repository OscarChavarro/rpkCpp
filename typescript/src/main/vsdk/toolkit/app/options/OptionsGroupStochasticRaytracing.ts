import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";
import { RayTracingLightMode } from "../../raycasting/stochasticRaytracing/RayTracingLightMode";
import { RayTracingRadMode } from "../../raycasting/stochasticRaytracing/RayTracingRadMode";
import { RayTracingSamplingMode } from "../../raycasting/stochasticRaytracing/RayTracingSamplingMode";
import { StochasticRayTracingState } from "../../raycasting/stochasticRaytracing/StochasticRayTracingState";
import { EnumDesc } from "./EnumDesc";
import { OptionTextUtils } from "./OptionTextUtils";

type EnumBinding<T extends number> = {
  target: TypedOption.Reference<T>;
  values: EnumDesc[];
  enumType: Record<string, number | string>;
};

export class OptionsGroupStochasticRaytracing {
  private static rayTracingRadianceModeValues: EnumDesc[] = [
    new EnumDesc(RayTracingRadMode.STORED_NONE, "none", 2),
    new EnumDesc(RayTracingRadMode.STORED_DIRECT, "direct", 2),
    new EnumDesc(RayTracingRadMode.STORED_INDIRECT, "indirect", 2),
    new EnumDesc(RayTracingRadMode.STORED_PHOTON_MAP, "photonmap", 2),
    new EnumDesc(0, null, 0),
  ];

  private static rayTracingLightModeValues: EnumDesc[] = [
    new EnumDesc(RayTracingLightMode.POWER_LIGHTS, "power", 2),
    new EnumDesc(RayTracingLightMode.IMPORTANT_LIGHTS, "important", 2),
    new EnumDesc(RayTracingLightMode.ALL_LIGHTS, "all", 2),
    new EnumDesc(0, null, 0),
  ];

  private static rayTracingSamplingModeValues: EnumDesc[] = [
    new EnumDesc(RayTracingSamplingMode.BRDF_SAMPLING, "bsdf", 2),
    new EnumDesc(RayTracingSamplingMode.CLASSICAL_SAMPLING, "classical", 2),
    new EnumDesc(0, null, 0),
  ];

  private constructor() {
  }

  private static parseEnumBinding<T extends number>(
    argc: number,
    argv: string[] | null,
    bindingValue: TypedOption.MutableValue<EnumBinding<T>>
  ): boolean {
    const binding = bindingValue.value;
    if (argc < 1 || argv === null || argv[0] === null || binding === null || binding.target === null || binding.values === null) {
      return false;
    }
    for (let i = 0; binding.values[i].name !== null; i++) {
      if (OptionTextUtils.equalsIgnoreCasePrefix(argv[0], binding.values[i].name, binding.values[i].abbrev)) {
        const ordinal = binding.values[i].value;
        if ((binding.enumType as Record<number, string | number>)[ordinal] === undefined) {
          return false;
        }
        binding.target.set(ordinal as T);
        return true;
      }
    }
    return false;
  }

  private static setIntTrue(value: TypedOption.MutableValue<number>): void {
    value.value = 1;
  }

  private static setIntFalse(value: TypedOption.MutableValue<number>): void {
    value.value = 0;
  }

  public static parse(argc: number[], argv: string[], stochasticRayTracingState: StochasticRayTracingState): void {
    const radModeBinding: EnumBinding<RayTracingRadMode> = {
      target: TypedOption.reference(() => stochasticRayTracingState.radMode, (v) => {
        stochasticRayTracingState.radMode = v;
      }),
      values: OptionsGroupStochasticRaytracing.rayTracingRadianceModeValues,
      enumType: RayTracingRadMode as unknown as Record<string, number | string>,
    };

    const lightModeBinding: EnumBinding<RayTracingLightMode> = {
      target: TypedOption.reference(() => stochasticRayTracingState.lightMode, (v) => {
        stochasticRayTracingState.lightMode = v;
      }),
      values: OptionsGroupStochasticRaytracing.rayTracingLightModeValues,
      enumType: RayTracingLightMode as unknown as Record<string, number | string>,
    };

    const samplingModeBinding: EnumBinding<RayTracingSamplingMode> = {
      target: TypedOption.reference(() => stochasticRayTracingState.reflectionSampling, (v) => {
        stochasticRayTracingState.reflectionSampling = v;
      }),
      values: OptionsGroupStochasticRaytracing.rayTracingSamplingModeValues,
      enumType: RayTracingSamplingMode as unknown as Record<string, number | string>,
    };

    const rtsSamplesPerPixelOpt = new TypedOption<number>(
      "-rts-samples-per-pixel",
      TypedOption.reference(() => stochasticRayTracingState.samplesPerPixel, (v) => {
        stochasticRayTracingState.samplesPerPixel = v;
      }),
      1,
      null,
      null
    );
    const rtsNoProgressiveOpt = new TypedOption<number>(
      "-rts-no-progressive",
      TypedOption.reference(() => stochasticRayTracingState.progressiveTracing, (v) => {
        stochasticRayTracingState.progressiveTracing = v;
      }),
      0,
      OptionsGroupStochasticRaytracing.setIntFalse,
      null
    );
    const rtsRadModeOpt = new TypedOption<EnumBinding<RayTracingRadMode>>(
      "-rts-rad-mode",
      TypedOption.valueRef(radModeBinding),
      1,
      null,
      OptionsGroupStochasticRaytracing.parseEnumBinding
    );
    const rtsNoLightSamplingOpt = new TypedOption<number>(
      "-rts-no-lightsampling",
      TypedOption.reference(() => stochasticRayTracingState.nextEvent, (v) => {
        stochasticRayTracingState.nextEvent = v;
      }),
      0,
      OptionsGroupStochasticRaytracing.setIntFalse,
      null
    );
    const rtsLightModeOpt = new TypedOption<EnumBinding<RayTracingLightMode>>(
      "-rts-l-mode",
      TypedOption.valueRef(lightModeBinding),
      1,
      null,
      OptionsGroupStochasticRaytracing.parseEnumBinding
    );
    const rtsLightSamplesOpt = new TypedOption<number>(
      "-rts-l-samples",
      TypedOption.reference(() => stochasticRayTracingState.nextEventSamples, (v) => {
        stochasticRayTracingState.nextEventSamples = v;
      }),
      1,
      null,
      null
    );
    const rtsScatterSamplesOpt = new TypedOption<number>(
      "-rts-scatter-samples",
      TypedOption.reference(() => stochasticRayTracingState.scatterSamples, (v) => {
        stochasticRayTracingState.scatterSamples = v;
      }),
      1,
      null,
      null
    );
    const rtsDoFdgOpt = new TypedOption<number>(
      "-rts-do-fdg",
      TypedOption.reference(() => stochasticRayTracingState.differentFirstDG, (v) => {
        stochasticRayTracingState.differentFirstDG = v;
      }),
      0,
      OptionsGroupStochasticRaytracing.setIntTrue,
      null
    );
    const rtsFdgSamplesOpt = new TypedOption<number>(
      "-rts-fdg-samples",
      TypedOption.reference(() => stochasticRayTracingState.firstDGSamples, (v) => {
        stochasticRayTracingState.firstDGSamples = v;
      }),
      1,
      null,
      null
    );
    const rtsSeparateSpecularOpt = new TypedOption<number>(
      "-rts-separate-specular",
      TypedOption.reference(() => stochasticRayTracingState.separateSpecular, (v) => {
        stochasticRayTracingState.separateSpecular = v;
      }),
      0,
      OptionsGroupStochasticRaytracing.setIntTrue,
      null
    );
    const rtsSamplingModeOpt = new TypedOption<EnumBinding<RayTracingSamplingMode>>(
      "-rts-s-mode",
      TypedOption.valueRef(samplingModeBinding),
      1,
      null,
      OptionsGroupStochasticRaytracing.parseEnumBinding
    );
    const rtsMinPathLengthOpt = new TypedOption<number>(
      "-rts-min-path-length",
      TypedOption.reference(() => stochasticRayTracingState.minPathDepth, (v) => {
        stochasticRayTracingState.minPathDepth = v;
      }),
      1,
      null,
      null
    );
    const rtsMaxPathLengthOpt = new TypedOption<number>(
      "-rts-max-path-length",
      TypedOption.reference(() => stochasticRayTracingState.maxPathDepth, (v) => {
        stochasticRayTracingState.maxPathDepth = v;
      }),
      1,
      null,
      null
    );
    const rtsNoDirectBackgroundOpt = new TypedOption<number>(
      "-rts-NOdirect-background-rad",
      TypedOption.reference(() => stochasticRayTracingState.backgroundDirect, (v) => {
        stochasticRayTracingState.backgroundDirect = v;
      }),
      0,
      OptionsGroupStochasticRaytracing.setIntFalse,
      null
    );
    const rtsNoIndirectBackgroundOpt = new TypedOption<number>(
      "-rts-NOindirect-background-rad",
      TypedOption.reference(() => stochasticRayTracingState.backgroundIndirect, (v) => {
        stochasticRayTracingState.backgroundIndirect = v;
      }),
      0,
      OptionsGroupStochasticRaytracing.setIntFalse,
      null
    );
    const stochasticRatTracerOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(rtsSamplesPerPixelOpt, 7),
      TypedOption.REGISTER_OPTION(rtsNoProgressiveOpt, 9),
      TypedOption.REGISTER_OPTION(rtsRadModeOpt, 8),
      TypedOption.REGISTER_OPTION(rtsNoLightSamplingOpt, 9),
      TypedOption.REGISTER_OPTION(rtsLightModeOpt, 8),
      TypedOption.REGISTER_OPTION(rtsLightSamplesOpt, 8),
      TypedOption.REGISTER_OPTION(rtsScatterSamplesOpt, 7),
      TypedOption.REGISTER_OPTION(rtsDoFdgOpt, 0),
      TypedOption.REGISTER_OPTION(rtsFdgSamplesOpt, 8),
      TypedOption.REGISTER_OPTION(rtsSeparateSpecularOpt, 8),
      TypedOption.REGISTER_OPTION(rtsSamplingModeOpt, 9),
      TypedOption.REGISTER_OPTION(rtsMinPathLengthOpt, 8),
      TypedOption.REGISTER_OPTION(rtsMaxPathLengthOpt, 8),
      TypedOption.REGISTER_OPTION(rtsNoDirectBackgroundOpt, 8),
      TypedOption.REGISTER_OPTION(rtsNoIndirectBackgroundOpt, 8),
    ];
    const stochasticRaytracerGroups = [
      new OptionGroup("stochasticRaytracer", stochasticRatTracerOptions, 15),
    ];
    OptionParser.parse(argc, argv, stochasticRaytracerGroups, 1);
  }
}
