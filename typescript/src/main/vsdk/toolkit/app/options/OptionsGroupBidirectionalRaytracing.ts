import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";
import { BidirectionalPathRaytracerConfig } from "../../raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig";
import { BidirectionalPathTracingState } from "../../raycasting/bidirectionalRaytracing/BidirectionalPathTracingState";
import { OptionTextUtils } from "./OptionTextUtils";

type FixedStringBinding = {
  target: TypedOption.Reference<string>;
  maxLength: number;
};

export class OptionsGroupBidirectionalRaytracing {
  private static regExpStringLength = BidirectionalPathRaytracerConfig.MAX_REGEXP_SIZE;

  private constructor() {
  }

  private static parseFixedStringBinding(
    argc: number,
    argv: string[] | null,
    bindingValue: TypedOption.MutableValue<FixedStringBinding>
  ): boolean {
    const binding = bindingValue.value;
    if (
      argc < 1
      || argv === null
      || argv[0] === null
      || binding === null
      || binding.target === null
      || binding.maxLength <= 0
    ) {
      return false;
    }
    let value = argv[0];
    let maxTextLength = binding.maxLength - 1;
    if (maxTextLength < 0) {
      maxTextLength = 0;
    }
    if (value.length > maxTextLength) {
      value = value.substring(0, maxTextLength);
    }
    binding.target.set(value);
    return true;
  }

  private static parseBoolInt(
    argc: number,
    argv: string[] | null,
    value: TypedOption.MutableValue<number>
  ): boolean {
    if (argc < 1 || argv === null || argv[0] === null) {
      return false;
    }
    const parsed = new OptionTextUtils.TypedIntValue(value.value);
    if (!OptionTextUtils.parseBoolInt(argv[0], parsed)) {
      return false;
    }
    value.value = parsed.value;
    return true;
  }

  private static setIntTrue(value: TypedOption.MutableValue<number>): void {
    value.value = 1;
  }

  private static setIntFalse(value: TypedOption.MutableValue<number>): void {
    value.value = 0;
  }

  public static parse(argc: number[], argv: string[], bidirectionalPathState: BidirectionalPathTracingState): void {
    const leBinding: FixedStringBinding = {
      target: TypedOption.reference(
        () => bidirectionalPathState.baseConfig.leRegExp,
        (v) => {
          bidirectionalPathState.baseConfig.leRegExp = v;
        }
      ),
      maxLength: OptionsGroupBidirectionalRaytracing.regExpStringLength,
    };
    const ldBinding: FixedStringBinding = {
      target: TypedOption.reference(
        () => bidirectionalPathState.baseConfig.ldRegExp,
        (v) => {
          bidirectionalPathState.baseConfig.ldRegExp = v;
        }
      ),
      maxLength: OptionsGroupBidirectionalRaytracing.regExpStringLength,
    };
    const liBinding: FixedStringBinding = {
      target: TypedOption.reference(
        () => bidirectionalPathState.baseConfig.liRegExp,
        (v) => {
          bidirectionalPathState.baseConfig.liRegExp = v;
        }
      ),
      maxLength: OptionsGroupBidirectionalRaytracing.regExpStringLength,
    };

    const bidirSamplesPerPixelOpt = new TypedOption<number>(
      "-bidir-samples-per-pixel",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.samplesPerPixel, (v) => {
        bidirectionalPathState.baseConfig.samplesPerPixel = v;
      }),
      1,
      null,
      null
    );
    const bidirNoProgressiveOpt = new TypedOption<number>(
      "-bidir-no-progressive",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.progressiveTracing, (v) => {
        bidirectionalPathState.baseConfig.progressiveTracing = v;
      }),
      0,
      OptionsGroupBidirectionalRaytracing.setIntFalse,
      null
    );
    const bidirMaxEyePathLengthOpt = new TypedOption<number>(
      "-bidir-max-eye-path-length",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.maximumEyePathDepth, (v) => {
        bidirectionalPathState.baseConfig.maximumEyePathDepth = v;
      }),
      1,
      null,
      null
    );
    const bidirMaxLightPathLengthOpt = new TypedOption<number>(
      "-bidir-max-light-path-length",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.maximumLightPathDepth, (v) => {
        bidirectionalPathState.baseConfig.maximumLightPathDepth = v;
      }),
      1,
      null,
      null
    );
    const bidirMaxPathLengthOpt = new TypedOption<number>(
      "-bidir-max-path-length",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.maximumPathDepth, (v) => {
        bidirectionalPathState.baseConfig.maximumPathDepth = v;
      }),
      1,
      null,
      null
    );
    const bidirMinPathLengthOpt = new TypedOption<number>(
      "-bidir-min-path-length",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.minimumPathDepth, (v) => {
        bidirectionalPathState.baseConfig.minimumPathDepth = v;
      }),
      1,
      null,
      null
    );
    const bidirNoLightImportanceOpt = new TypedOption<number>(
      "-bidir-no-light-importance",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.sampleImportantLights, (v) => {
        bidirectionalPathState.baseConfig.sampleImportantLights = v;
      }),
      0,
      OptionsGroupBidirectionalRaytracing.setIntFalse,
      null
    );
    const bidirUseRegexpOpt = new TypedOption<number>(
      "-bidir-use-regexp",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.useSpars, (v) => {
        bidirectionalPathState.baseConfig.useSpars = v;
      }),
      0,
      OptionsGroupBidirectionalRaytracing.setIntTrue,
      null
    );
    const bidirUseEmittedOpt = new TypedOption<number>(
      "-bidir-use-emitted",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.doLe, (v) => {
        bidirectionalPathState.baseConfig.doLe = v;
      }),
      1,
      null,
      OptionsGroupBidirectionalRaytracing.parseBoolInt
    );
    const bidirRexpEmittedOpt = new TypedOption<FixedStringBinding>(
      "-bidir-rexp-emitted",
      TypedOption.valueRef(leBinding),
      1,
      null,
      OptionsGroupBidirectionalRaytracing.parseFixedStringBinding
    );
    const bidirRegDirectOpt = new TypedOption<number>(
      "-bidir-reg-direct",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.doLD, (v) => {
        bidirectionalPathState.baseConfig.doLD = v;
      }),
      1,
      null,
      OptionsGroupBidirectionalRaytracing.parseBoolInt
    );
    const bidirRexpDirectOpt = new TypedOption<FixedStringBinding>(
      "-bidir-rexp-direct",
      TypedOption.valueRef(ldBinding),
      1,
      null,
      OptionsGroupBidirectionalRaytracing.parseFixedStringBinding
    );
    const bidirRegIndirectOpt = new TypedOption<number>(
      "-bidir-reg-indirect",
      TypedOption.reference(() => bidirectionalPathState.baseConfig.doLI, (v) => {
        bidirectionalPathState.baseConfig.doLI = v;
      }),
      1,
      null,
      OptionsGroupBidirectionalRaytracing.parseBoolInt
    );
    const bidirRexpIndirectOpt = new TypedOption<FixedStringBinding>(
      "-bidir-rexp-indirect",
      TypedOption.valueRef(liBinding),
      1,
      null,
      OptionsGroupBidirectionalRaytracing.parseFixedStringBinding
    );
    const bidirectionalOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(bidirSamplesPerPixelOpt, 8),
      TypedOption.REGISTER_OPTION(bidirNoProgressiveOpt, 11),
      TypedOption.REGISTER_OPTION(bidirMaxEyePathLengthOpt, 12),
      TypedOption.REGISTER_OPTION(bidirMaxLightPathLengthOpt, 12),
      TypedOption.REGISTER_OPTION(bidirMaxPathLengthOpt, 12),
      TypedOption.REGISTER_OPTION(bidirMinPathLengthOpt, 12),
      TypedOption.REGISTER_OPTION(bidirNoLightImportanceOpt, 11),
      TypedOption.REGISTER_OPTION(bidirUseRegexpOpt, 12),
      TypedOption.REGISTER_OPTION(bidirUseEmittedOpt, 12),
      TypedOption.REGISTER_OPTION(bidirRexpEmittedOpt, 13),
      TypedOption.REGISTER_OPTION(bidirRegDirectOpt, 12),
      TypedOption.REGISTER_OPTION(bidirRexpDirectOpt, 13),
      TypedOption.REGISTER_OPTION(bidirRegIndirectOpt, 12),
      TypedOption.REGISTER_OPTION(bidirRexpIndirectOpt, 13),
    ];
    const bidirectionalGroups = [
      new OptionGroup("bidirectional", bidirectionalOptions, 14),
    ];
    OptionParser.parse(argc, argv, bidirectionalGroups, 1);
  }
}
