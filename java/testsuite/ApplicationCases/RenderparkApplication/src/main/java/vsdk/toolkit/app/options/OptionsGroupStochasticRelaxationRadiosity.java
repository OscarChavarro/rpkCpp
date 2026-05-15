package vsdk.toolkit.app.options;

import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.raycasting.stochasticRaytracing.ElementHierarchyState;
import vsdk.toolkit.raycasting.stochasticRaytracing.HierarchyClusteringMode;
import vsdk.toolkit.raycasting.stochasticRaytracing.Sampler4DSequence;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRaytracingApproximation;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRelaxation;
import vsdk.toolkit.raycasting.stochasticRaytracing.WhatToShow;

public final class OptionsGroupStochasticRelaxationRadiosity {
    private static final class EnumBinding<T extends Enum<T>> {
        public TypedOption.Reference<T> target;
        public EnumDesc[] values;
        public Class<T> enumType;
    }

    private static EnumDesc[] approximateValues = new EnumDesc[] {
        new EnumDesc(StochasticRaytracingApproximation.CONSTANT.ordinal(), "constant", 2),
        new EnumDesc(StochasticRaytracingApproximation.LINEAR.ordinal(), "linear", 2),
        new EnumDesc(StochasticRaytracingApproximation.BI_LINEAR.ordinal(), "bilinear", 2),
        new EnumDesc(StochasticRaytracingApproximation.QUADRATIC.ordinal(), "quadratic", 2),
        new EnumDesc(StochasticRaytracingApproximation.CUBIC.ordinal(), "cubic", 2),
        new EnumDesc(0, null, 0)
    };

    private static EnumDesc[] clusteringValues = new EnumDesc[] {
        new EnumDesc(HierarchyClusteringMode.NO_CLUSTERING.ordinal(), "none", 2),
        new EnumDesc(HierarchyClusteringMode.ISOTROPIC_CLUSTERING.ordinal(), "isotropic", 2),
        new EnumDesc(HierarchyClusteringMode.ORIENTED_CLUSTERING.ordinal(), "oriented", 2),
        new EnumDesc(0, null, 0)
    };

    private static EnumDesc[] sequenceValues = new EnumDesc[] {
        new EnumDesc(Sampler4DSequence.RANDOM.ordinal(), "PseudoRandom", 2),
        new EnumDesc(Sampler4DSequence.HALTON.ordinal(), "Halton", 2),
        new EnumDesc(Sampler4DSequence.NIEDERREITER.ordinal(), "Niederreiter", 2),
        new EnumDesc(0, null, 0)
    };

    private static EnumDesc[] showWhatValues = new EnumDesc[] {
        new EnumDesc(WhatToShow.SHOW_TOTAL_RADIANCE.ordinal(), "total-radiance", 2),
        new EnumDesc(WhatToShow.SHOW_INDIRECT_RADIANCE.ordinal(), "indirect-radiance", 2),
        new EnumDesc(WhatToShow.SHOW_IMPORTANCE.ordinal(), "importance", 2),
        new EnumDesc(0, null, 0)
    };

    private OptionsGroupStochasticRelaxationRadiosity() {
    }

    private static <T extends Enum<T>> boolean parseEnumBinding(
        int argc,
        String[] argv,
        TypedOption.MutableValue<EnumBinding<T>> bindingValue)
    {
        EnumBinding<T> binding = bindingValue.value;
        if ( argc < 1 || argv == null || argv[0] == null || binding == null || binding.target == null || binding.values == null ) {
            return false;
        }
        for ( int i = 0; binding.values[i].name != null; i++ ) {
            if ( OptionTextUtils.equalsIgnoreCasePrefix(argv[0], binding.values[i].name, binding.values[i].abbrev) ) {
                T[] constants = binding.enumType.getEnumConstants();
                int ordinal = binding.values[i].value;
                if ( ordinal < 0 || ordinal >= constants.length ) {
                    return false;
                }
                binding.target.set(constants[ordinal]);
                return true;
            }
        }
        return false;
    }

    private static boolean parseBoolInt(
        int argc,
        String[] argv,
        TypedOption.MutableValue<Integer> value)
    {
        if ( argc < 1 || argv == null || argv[0] == null ) {
            return false;
        }

        OptionTextUtils.TypedIntValue parsed = new OptionTextUtils.TypedIntValue(value.value);
        if ( !OptionTextUtils.parseBoolInt(argv[0], parsed) ) {
            return false;
        }
        value.value = parsed.value;
        return true;
    }

    public static void parse(
        int[] argc,
        String[] argv,
        StochasticRelaxation stochasticRelaxationState,
        ElementHierarchyState elementHierarchyState)
    {
        EnumBinding<Sampler4DSequence> sequenceBinding = new EnumBinding<>();
        sequenceBinding.target = TypedOption.reference(() -> stochasticRelaxationState.sequence, v -> stochasticRelaxationState.sequence = v);
        sequenceBinding.values = sequenceValues;
        sequenceBinding.enumType = Sampler4DSequence.class;

        EnumBinding<StochasticRaytracingApproximation> approximationBinding = new EnumBinding<>();
        approximationBinding.target = TypedOption.reference(() -> stochasticRelaxationState.approximationOrderType, v -> stochasticRelaxationState.approximationOrderType = v);
        approximationBinding.values = approximateValues;
        approximationBinding.enumType = StochasticRaytracingApproximation.class;

        EnumBinding<HierarchyClusteringMode> clusteringBinding = new EnumBinding<>();
        clusteringBinding.target = TypedOption.reference(() -> elementHierarchyState.clustering, v -> elementHierarchyState.clustering = v);
        clusteringBinding.values = clusteringValues;
        clusteringBinding.enumType = HierarchyClusteringMode.class;

        EnumBinding<WhatToShow> showBinding = new EnumBinding<>();
        showBinding.target = TypedOption.reference(() -> stochasticRelaxationState.show, v -> stochasticRelaxationState.show = v);
        showBinding.values = showWhatValues;
        showBinding.enumType = WhatToShow.class;

        TypedOption<Integer> srrRayUnitsOpt = new TypedOption<>(
            "-srr-ray-units",
            TypedOption.reference(() -> stochasticRelaxationState.rayUnitsPerIt, v -> stochasticRelaxationState.rayUnitsPerIt = v),
            1,
            null,
            null);
        TypedOption<Integer> srrBidirectionalOpt = new TypedOption<>(
            "-srr-bidirectional",
            TypedOption.reference(() -> stochasticRelaxationState.bidirectionalTransfers, v -> stochasticRelaxationState.bidirectionalTransfers = v),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseBoolInt);
        TypedOption<Integer> srrControlVariateOpt = new TypedOption<>(
            "-srr-control-variate",
            TypedOption.reference(() -> stochasticRelaxationState.constantControlVariate, v -> stochasticRelaxationState.constantControlVariate = v),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseBoolInt);
        TypedOption<Integer> srrIndirectOnlyOpt = new TypedOption<>(
            "-srr-indirect-only",
            TypedOption.reference(() -> stochasticRelaxationState.indirectOnly, v -> stochasticRelaxationState.indirectOnly = v),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseBoolInt);
        TypedOption<Integer> srrImportanceDrivenOpt = new TypedOption<>(
            "-srr-importance-driven",
            TypedOption.reference(() -> stochasticRelaxationState.importanceDriven, v -> stochasticRelaxationState.importanceDriven = v),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseBoolInt);
        TypedOption<EnumBinding<Sampler4DSequence>> srrSequenceOpt = new TypedOption<>(
            "-srr-sampling-sequence",
            TypedOption.valueRef(sequenceBinding),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseEnumBinding);
        TypedOption<EnumBinding<StochasticRaytracingApproximation>> srrApproximationOpt = new TypedOption<>(
            "-srr-approximation",
            TypedOption.valueRef(approximationBinding),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseEnumBinding);
        TypedOption<Integer> srrHierarchicalOpt = new TypedOption<>(
            "-srr-hierarchical",
            TypedOption.reference(() -> elementHierarchyState.do_h_meshing, v -> elementHierarchyState.do_h_meshing = v),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseBoolInt);
        TypedOption<EnumBinding<HierarchyClusteringMode>> srrClusteringOpt = new TypedOption<>(
            "-srr-clustering",
            TypedOption.valueRef(clusteringBinding),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseEnumBinding);
        TypedOption<Float> srrEpsilonOpt = new TypedOption<>(
            "-srr-epsilon",
            TypedOption.reference(() -> elementHierarchyState.epsilon, v -> elementHierarchyState.epsilon = v),
            1,
            null,
            null);
        TypedOption<Float> srrMinAreaOpt = new TypedOption<>(
            "-srr-minarea",
            TypedOption.reference(() -> elementHierarchyState.minimumArea, v -> elementHierarchyState.minimumArea = v),
            1,
            null,
            null);
        TypedOption<EnumBinding<WhatToShow>> srrDisplayOpt = new TypedOption<>(
            "-srr-display",
            TypedOption.valueRef(showBinding),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseEnumBinding);
        TypedOption<Integer> srrDiscardIncrementalOpt = new TypedOption<>(
            "-srr-discard-incremental",
            TypedOption.reference(() -> stochasticRelaxationState.discardIncremental, v -> stochasticRelaxationState.discardIncremental = v),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseBoolInt);
        TypedOption<Integer> srrIncrementalImportanceOpt = new TypedOption<>(
            "-srr-incremental-uses-importance",
            TypedOption.reference(() -> stochasticRelaxationState.incrementalUsesImportance, v -> stochasticRelaxationState.incrementalUsesImportance = v),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseBoolInt);
        TypedOption<Integer> srrNaiveMergingOpt = new TypedOption<>(
            "-srr-naive-merging",
            TypedOption.reference(() -> stochasticRelaxationState.naiveMerging, v -> stochasticRelaxationState.naiveMerging = v),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseBoolInt);
        TypedOption<Integer> srrNonDiffuseFirstShotOpt = new TypedOption<>(
            "-srr-nondiffuse-first-shot",
            TypedOption.reference(() -> stochasticRelaxationState.doNonDiffuseFirstShot, v -> stochasticRelaxationState.doNonDiffuseFirstShot = v),
            1,
            null,
            OptionsGroupStochasticRelaxationRadiosity::parseBoolInt);
        TypedOption<Integer> srrInitialLsSamplesOpt = new TypedOption<>(
            "-srr-initial-ls-samples",
            TypedOption.reference(() -> stochasticRelaxationState.initialLightSourceSamples, v -> stochasticRelaxationState.initialLightSourceSamples = v),
            1,
            null,
            null);
        OptionBase[] srrOptions = new OptionBase[] {
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
            TypedOption.REGISTER_OPTION(srrInitialLsSamplesOpt, 7)
        };
        OptionGroup[] srrGroups = new OptionGroup[] {
            new OptionGroup("stochasticRelaxation", srrOptions, 17)
        };
        OptionParser.parse(argc, argv, srrGroups, 1);
    }
}
