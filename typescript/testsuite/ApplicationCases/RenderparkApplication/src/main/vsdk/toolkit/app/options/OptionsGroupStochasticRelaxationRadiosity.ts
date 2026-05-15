import { OptionBase } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/OptionBase";
import { OptionGroup } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/OptionGroup";
import { OptionParser } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/OptionParser";
import { TypedOption } from "vitral/dist/vsdk/toolkit/common/commandLineOptions/TypedOption";
import { ElementHierarchyState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState";
import { HierarchyClusteringMode } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/HierarchyClusteringMode";
import { Sampler4DSequence } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/Sampler4DSequence";
import { StochasticRaytracingApproximation } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRaytracingApproximation";
import { StochasticRelaxation } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation";
import { WhatToShow } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/WhatToShow";
import { EnumDesc } from "./EnumDesc";
import { OptionTextUtils } from "./OptionTextUtils";

type EnumBinding<T extends number> = {
  target: TypedOption.Reference<T>;
  values: EnumDesc[];
  enumType: Record<string, number | string>;
};

export class OptionsGroupStochasticRelaxationRadiosity {
  private static approximateValues: EnumDesc[] = [
    new EnumDesc(StochasticRaytracingApproximation.CONSTANT, "constant", 2),
    new EnumDesc(StochasticRaytracingApproximation.LINEAR, "linear", 2),
    new EnumDesc(StochasticRaytracingApproximation.BI_LINEAR, "bilinear", 2),
    new EnumDesc(StochasticRaytracingApproximation.QUADRATIC, "quadratic", 2),
    new EnumDesc(StochasticRaytracingApproximation.CUBIC, "cubic", 2),
    new EnumDesc(0, null, 0),
  ];

  private static clusteringValues: EnumDesc[] = [
    new EnumDesc(HierarchyClusteringMode.NO_CLUSTERING, "none", 2),
    new EnumDesc(HierarchyClusteringMode.ISOTROPIC_CLUSTERING, "isotropic", 2),
    new EnumDesc(HierarchyClusteringMode.ORIENTED_CLUSTERING, "oriented", 2),
    new EnumDesc(0, null, 0),
  ];

  private static sequenceValues: EnumDesc[] = [
    new EnumDesc(Sampler4DSequence.RANDOM, "PseudoRandom", 2),
    new EnumDesc(Sampler4DSequence.HALTON, "Halton", 2),
    new EnumDesc(Sampler4DSequence.NIEDERREITER, "Niederreiter", 2),
    new EnumDesc(0, null, 0),
  ];

  private static showWhatValues: EnumDesc[] = [
    new EnumDesc(WhatToShow.SHOW_TOTAL_RADIANCE, "total-radiance", 2),
    new EnumDesc(WhatToShow.SHOW_INDIRECT_RADIANCE, "indirect-radiance", 2),
    new EnumDesc(WhatToShow.SHOW_IMPORTANCE, "importance", 2),
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

  public static parse(
    argc: number[],
    argv: string[],
    stochasticRelaxationState: StochasticRelaxation,
    elementHierarchyState: ElementHierarchyState
  ): void {
    const sequenceBinding: EnumBinding<Sampler4DSequence> = {
      target: TypedOption.reference(() => stochasticRelaxationState.sequence as Sampler4DSequence, (v) => {
        stochasticRelaxationState.sequence = v;
      }),
      values: OptionsGroupStochasticRelaxationRadiosity.sequenceValues,
      enumType: Sampler4DSequence as unknown as Record<string, number | string>,
    };

    const approximationBinding: EnumBinding<StochasticRaytracingApproximation> = {
      target: TypedOption.reference(
        () => stochasticRelaxationState.approximationOrderType as StochasticRaytracingApproximation,
        (v) => {
          stochasticRelaxationState.approximationOrderType = v;
        }
      ),
      values: OptionsGroupStochasticRelaxationRadiosity.approximateValues,
      enumType: StochasticRaytracingApproximation as unknown as Record<string, number | string>,
    };

    const clusteringBinding: EnumBinding<HierarchyClusteringMode> = {
      target: TypedOption.reference(() => elementHierarchyState.clustering as HierarchyClusteringMode, (v) => {
        elementHierarchyState.clustering = v;
      }),
      values: OptionsGroupStochasticRelaxationRadiosity.clusteringValues,
      enumType: HierarchyClusteringMode as unknown as Record<string, number | string>,
    };

    const showBinding: EnumBinding<WhatToShow> = {
      target: TypedOption.reference(() => stochasticRelaxationState.show as WhatToShow, (v) => {
        stochasticRelaxationState.show = v;
      }),
      values: OptionsGroupStochasticRelaxationRadiosity.showWhatValues,
      enumType: WhatToShow as unknown as Record<string, number | string>,
    };

    const srrRayUnitsOpt = new TypedOption<number>(
      "-srr-ray-units",
      TypedOption.reference(() => stochasticRelaxationState.rayUnitsPerIt, (v) => {
        stochasticRelaxationState.rayUnitsPerIt = v;
      }),
      1,
      null,
      null
    );
    const srrBidirectionalOpt = new TypedOption<number>(
      "-srr-bidirectional",
      TypedOption.reference(() => stochasticRelaxationState.bidirectionalTransfers, (v) => {
        stochasticRelaxationState.bidirectionalTransfers = v;
      }),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseBoolInt
    );
    const srrControlVariateOpt = new TypedOption<number>(
      "-srr-control-variate",
      TypedOption.reference(() => stochasticRelaxationState.constantControlVariate, (v) => {
        stochasticRelaxationState.constantControlVariate = v;
      }),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseBoolInt
    );
    const srrIndirectOnlyOpt = new TypedOption<number>(
      "-srr-indirect-only",
      TypedOption.reference(() => stochasticRelaxationState.indirectOnly, (v) => {
        stochasticRelaxationState.indirectOnly = v;
      }),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseBoolInt
    );
    const srrImportanceDrivenOpt = new TypedOption<number>(
      "-srr-importance-driven",
      TypedOption.reference(() => stochasticRelaxationState.importanceDriven, (v) => {
        stochasticRelaxationState.importanceDriven = v;
      }),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseBoolInt
    );
    const srrSequenceOpt = new TypedOption<EnumBinding<Sampler4DSequence>>(
      "-srr-sampling-sequence",
      TypedOption.valueRef(sequenceBinding),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseEnumBinding
    );
    const srrApproximationOpt = new TypedOption<EnumBinding<StochasticRaytracingApproximation>>(
      "-srr-approximation",
      TypedOption.valueRef(approximationBinding),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseEnumBinding
    );
    const srrHierarchicalOpt = new TypedOption<number>(
      "-srr-hierarchical",
      TypedOption.reference(() => elementHierarchyState.do_h_meshing, (v) => {
        elementHierarchyState.do_h_meshing = v;
      }),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseBoolInt
    );
    const srrClusteringOpt = new TypedOption<EnumBinding<HierarchyClusteringMode>>(
      "-srr-clustering",
      TypedOption.valueRef(clusteringBinding),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseEnumBinding
    );
    const srrEpsilonOpt = new TypedOption<number>(
      "-srr-epsilon",
      TypedOption.reference(() => elementHierarchyState.epsilon, (v) => {
        elementHierarchyState.epsilon = v;
      }),
      1,
      null,
      null
    );
    const srrMinAreaOpt = new TypedOption<number>(
      "-srr-minarea",
      TypedOption.reference(() => elementHierarchyState.minimumArea, (v) => {
        elementHierarchyState.minimumArea = v;
      }),
      1,
      null,
      null
    );
    const srrDisplayOpt = new TypedOption<EnumBinding<WhatToShow>>(
      "-srr-display",
      TypedOption.valueRef(showBinding),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseEnumBinding
    );
    const srrDiscardIncrementalOpt = new TypedOption<number>(
      "-srr-discard-incremental",
      TypedOption.reference(() => stochasticRelaxationState.discardIncremental, (v) => {
        stochasticRelaxationState.discardIncremental = v;
      }),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseBoolInt
    );
    const srrIncrementalImportanceOpt = new TypedOption<number>(
      "-srr-incremental-uses-importance",
      TypedOption.reference(() => stochasticRelaxationState.incrementalUsesImportance, (v) => {
        stochasticRelaxationState.incrementalUsesImportance = v;
      }),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseBoolInt
    );
    const srrNaiveMergingOpt = new TypedOption<number>(
      "-srr-naive-merging",
      TypedOption.reference(() => stochasticRelaxationState.naiveMerging, (v) => {
        stochasticRelaxationState.naiveMerging = v;
      }),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseBoolInt
    );
    const srrNonDiffuseFirstShotOpt = new TypedOption<number>(
      "-srr-nondiffuse-first-shot",
      TypedOption.reference(() => stochasticRelaxationState.doNonDiffuseFirstShot, (v) => {
        stochasticRelaxationState.doNonDiffuseFirstShot = v;
      }),
      1,
      null,
      OptionsGroupStochasticRelaxationRadiosity.parseBoolInt
    );
    const srrInitialLsSamplesOpt = new TypedOption<number>(
      "-srr-initial-ls-samples",
      TypedOption.reference(() => stochasticRelaxationState.initialLightSourceSamples, (v) => {
        stochasticRelaxationState.initialLightSourceSamples = v;
      }),
      1,
      null,
      null
    );
    const srrOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(srrRayUnitsOpt, 8),
      TypedOption.REGISTER_OPTION(srrBidirectionalOpt, 7),
      TypedOption.REGISTER_OPTION(srrControlVariateOpt, 7),
      TypedOption.REGISTER_OPTION(srrIndirectOnlyOpt, 7),
      TypedOption.REGISTER_OPTION(srrImportanceDrivenOpt, 7),
      TypedOption.REGISTER_OPTION(srrSequenceOpt, 7),
      TypedOption.REGISTER_OPTION(srrApproximationOpt, 7),
      TypedOption.REGISTER_OPTION(srrHierarchicalOpt, 7),
      TypedOption.REGISTER_OPTION(srrClusteringOpt, 7),
      TypedOption.REGISTER_OPTION(srrEpsilonOpt, 7),
      TypedOption.REGISTER_OPTION(srrMinAreaOpt, 7),
      TypedOption.REGISTER_OPTION(srrDisplayOpt, 7),
      TypedOption.REGISTER_OPTION(srrDiscardIncrementalOpt, 7),
      TypedOption.REGISTER_OPTION(srrIncrementalImportanceOpt, 7),
      TypedOption.REGISTER_OPTION(srrNaiveMergingOpt, 7),
      TypedOption.REGISTER_OPTION(srrNonDiffuseFirstShotOpt, 7),
      TypedOption.REGISTER_OPTION(srrInitialLsSamplesOpt, 7),
    ];
    const srrGroups = [
      new OptionGroup("stochasticRelaxation", srrOptions, 17),
    ];
    OptionParser.parse(argc, argv, srrGroups, 1);
  }
}
