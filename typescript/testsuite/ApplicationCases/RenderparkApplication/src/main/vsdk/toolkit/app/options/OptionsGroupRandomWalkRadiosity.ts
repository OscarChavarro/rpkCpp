import { OptionBase } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/OptionBase";
import { OptionGroup } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/OptionGroup";
import { OptionParser } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/OptionParser";
import { TypedOption } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/TypedOption";
import { RandomWalkEstimatorKind } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/RandomWalkEstimatorKind";
import { RandomWalkEstimatorType } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/RandomWalkEstimatorType";
import { Sampler4DSequence } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/Sampler4DSequence";
import { StochasticRaytracingApproximation } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingApproximation";
import { StochasticRelaxation } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation";
import { EnumDesc } from "./EnumDesc";
import { OptionTextUtils } from "./OptionTextUtils";

type EnumBinding<T extends number> = {
  target: TypedOption.Reference<T>;
  values: EnumDesc[];
  enumType: Record<string, number | string>;
};

export class OptionsGroupRandomWalkRadiosity {
  private static approximateValues: EnumDesc[] = [
    new EnumDesc(StochasticRaytracingApproximation.CONSTANT, "constant", 2),
    new EnumDesc(StochasticRaytracingApproximation.LINEAR, "linear", 2),
    new EnumDesc(StochasticRaytracingApproximation.BI_LINEAR, "bilinear", 2),
    new EnumDesc(StochasticRaytracingApproximation.QUADRATIC, "quadratic", 2),
    new EnumDesc(StochasticRaytracingApproximation.CUBIC, "cubic", 2),
    new EnumDesc(0, null, 0),
  ];

  private static sequenceValues: EnumDesc[] = [
    new EnumDesc(Sampler4DSequence.RANDOM, "PseudoRandom", 2),
    new EnumDesc(Sampler4DSequence.HALTON, "Halton", 2),
    new EnumDesc(Sampler4DSequence.NIEDERREITER, "Niederreiter", 2),
    new EnumDesc(0, null, 0),
  ];

  private static estimatorTypeValues: EnumDesc[] = [
    new EnumDesc(RandomWalkEstimatorType.RW_SHOOTING, "Shooting", 2),
    new EnumDesc(RandomWalkEstimatorType.RW_GATHERING, "Gathering", 2),
    new EnumDesc(0, null, 0),
  ];

  private static estimatorKindValues: EnumDesc[] = [
    new EnumDesc(RandomWalkEstimatorKind.RW_COLLISION, "Collision", 2),
    new EnumDesc(RandomWalkEstimatorKind.RW_ABSORPTION, "Absorption", 2),
    new EnumDesc(RandomWalkEstimatorKind.RW_SURVIVAL, "Survival", 2),
    new EnumDesc(RandomWalkEstimatorKind.RW_LAST_BUT_NTH, "Last-but-N", 2),
    new EnumDesc(RandomWalkEstimatorKind.RW_N_LAST, "Last-N", 2),
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

  public static parse(argc: number[], argv: string[], stochasticRelaxationState: StochasticRelaxation): void {
    const sequenceBinding: EnumBinding<Sampler4DSequence> = {
      target: TypedOption.reference(() => stochasticRelaxationState.sequence as Sampler4DSequence, (v) => {
        stochasticRelaxationState.sequence = v;
      }),
      values: OptionsGroupRandomWalkRadiosity.sequenceValues,
      enumType: Sampler4DSequence as unknown as Record<string, number | string>,
    };

    const approximationBinding: EnumBinding<StochasticRaytracingApproximation> = {
      target: TypedOption.reference(
        () => stochasticRelaxationState.approximationOrderType as StochasticRaytracingApproximation,
        (v) => {
          stochasticRelaxationState.approximationOrderType = v;
        }
      ),
      values: OptionsGroupRandomWalkRadiosity.approximateValues,
      enumType: StochasticRaytracingApproximation as unknown as Record<string, number | string>,
    };

    const estimatorTypeBinding: EnumBinding<RandomWalkEstimatorType> = {
      target: TypedOption.reference(
        () => stochasticRelaxationState.randomWalkEstimatorType as RandomWalkEstimatorType,
        (v) => {
          stochasticRelaxationState.randomWalkEstimatorType = v;
        }
      ),
      values: OptionsGroupRandomWalkRadiosity.estimatorTypeValues,
      enumType: RandomWalkEstimatorType as unknown as Record<string, number | string>,
    };

    const estimatorKindBinding: EnumBinding<RandomWalkEstimatorKind> = {
      target: TypedOption.reference(
        () => stochasticRelaxationState.randomWalkEstimatorKind as RandomWalkEstimatorKind,
        (v) => {
          stochasticRelaxationState.randomWalkEstimatorKind = v;
        }
      ),
      values: OptionsGroupRandomWalkRadiosity.estimatorKindValues,
      enumType: RandomWalkEstimatorKind as unknown as Record<string, number | string>,
    };

    const rwrRayUnitsOpt = new TypedOption<number>(
      "-rwr-ray-units",
      TypedOption.reference(() => stochasticRelaxationState.rayUnitsPerIt, (v) => {
        stochasticRelaxationState.rayUnitsPerIt = v;
      }),
      1,
      null,
      null
    );
    const rwrContinuousOpt = new TypedOption<number>(
      "-rwr-continuous",
      TypedOption.reference(() => stochasticRelaxationState.continuousRandomWalk, (v) => {
        stochasticRelaxationState.continuousRandomWalk = v;
      }),
      1,
      null,
      OptionsGroupRandomWalkRadiosity.parseBoolInt
    );
    const rwrControlVariateOpt = new TypedOption<number>(
      "-rwr-control-variate",
      TypedOption.reference(() => stochasticRelaxationState.constantControlVariate, (v) => {
        stochasticRelaxationState.constantControlVariate = v;
      }),
      1,
      null,
      OptionsGroupRandomWalkRadiosity.parseBoolInt
    );
    const rwrIndirectOnlyOpt = new TypedOption<number>(
      "-rwr-indirect-only",
      TypedOption.reference(() => stochasticRelaxationState.indirectOnly, (v) => {
        stochasticRelaxationState.indirectOnly = v;
      }),
      1,
      null,
      OptionsGroupRandomWalkRadiosity.parseBoolInt
    );
    const rwrSequenceOpt = new TypedOption<EnumBinding<Sampler4DSequence>>(
      "-rwr-sampling-sequence",
      TypedOption.valueRef(sequenceBinding),
      1,
      null,
      OptionsGroupRandomWalkRadiosity.parseEnumBinding
    );
    const rwrApproximationOpt = new TypedOption<EnumBinding<StochasticRaytracingApproximation>>(
      "-rwr-approximation",
      TypedOption.valueRef(approximationBinding),
      1,
      null,
      OptionsGroupRandomWalkRadiosity.parseEnumBinding
    );
    const rwrEstimatorOpt = new TypedOption<EnumBinding<RandomWalkEstimatorType>>(
      "-rwr-estimator",
      TypedOption.valueRef(estimatorTypeBinding),
      1,
      null,
      OptionsGroupRandomWalkRadiosity.parseEnumBinding
    );
    const rwrScoreOpt = new TypedOption<EnumBinding<RandomWalkEstimatorKind>>(
      "-rwr-score",
      TypedOption.valueRef(estimatorKindBinding),
      1,
      null,
      OptionsGroupRandomWalkRadiosity.parseEnumBinding
    );
    const rwrNumlastOpt = new TypedOption<number>(
      "-rwr-numlast",
      TypedOption.reference(() => stochasticRelaxationState.randomWalkNumLast, (v) => {
        stochasticRelaxationState.randomWalkNumLast = v;
      }),
      1,
      null,
      null
    );
    const rwrOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(rwrRayUnitsOpt, 8),
      TypedOption.REGISTER_OPTION(rwrContinuousOpt, 7),
      TypedOption.REGISTER_OPTION(rwrControlVariateOpt, 7),
      TypedOption.REGISTER_OPTION(rwrIndirectOnlyOpt, 7),
      TypedOption.REGISTER_OPTION(rwrSequenceOpt, 7),
      TypedOption.REGISTER_OPTION(rwrApproximationOpt, 7),
      TypedOption.REGISTER_OPTION(rwrEstimatorOpt, 7),
      TypedOption.REGISTER_OPTION(rwrScoreOpt, 7),
      TypedOption.REGISTER_OPTION(rwrNumlastOpt, 12),
    ];
    const rwrGroups = [
      new OptionGroup("randomWalk", rwrOptions, 9),
    ];
    OptionParser.parse(argc, argv, rwrGroups, 1);
  }
}
