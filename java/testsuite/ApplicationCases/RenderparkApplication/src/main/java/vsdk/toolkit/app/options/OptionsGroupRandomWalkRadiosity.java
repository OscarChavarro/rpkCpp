package vsdk.toolkit.app.options;

import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.raycasting.stochasticRaytracing.RandomWalkEstimatorKind;
import vsdk.toolkit.raycasting.stochasticRaytracing.RandomWalkEstimatorType;
import vsdk.toolkit.raycasting.stochasticRaytracing.Sampler4DSequence;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRaytracingApproximation;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRelaxation;

public final class OptionsGroupRandomWalkRadiosity {
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

    private static EnumDesc[] sequenceValues = new EnumDesc[] {
        new EnumDesc(Sampler4DSequence.RANDOM.ordinal(), "PseudoRandom", 2),
        new EnumDesc(Sampler4DSequence.HALTON.ordinal(), "Halton", 2),
        new EnumDesc(Sampler4DSequence.NIEDERREITER.ordinal(), "Niederreiter", 2),
        new EnumDesc(0, null, 0)
    };

    private static EnumDesc[] estimatorTypeValues = new EnumDesc[] {
        new EnumDesc(RandomWalkEstimatorType.RW_SHOOTING.ordinal(), "Shooting", 2),
        new EnumDesc(RandomWalkEstimatorType.RW_GATHERING.ordinal(), "Gathering", 2),
        new EnumDesc(0, null, 0)
    };

    private static EnumDesc[] estimatorKindValues = new EnumDesc[] {
        new EnumDesc(RandomWalkEstimatorKind.RW_COLLISION.ordinal(), "Collision", 2),
        new EnumDesc(RandomWalkEstimatorKind.RW_ABSORPTION.ordinal(), "Absorption", 2),
        new EnumDesc(RandomWalkEstimatorKind.RW_SURVIVAL.ordinal(), "Survival", 2),
        new EnumDesc(RandomWalkEstimatorKind.RW_LAST_BUT_NTH.ordinal(), "Last-but-N", 2),
        new EnumDesc(RandomWalkEstimatorKind.RW_N_LAST.ordinal(), "Last-N", 2),
        new EnumDesc(0, null, 0)
    };

    private OptionsGroupRandomWalkRadiosity() {
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
        StochasticRelaxation stochasticRelaxationState)
    {
        EnumBinding<Sampler4DSequence> sequenceBinding = new EnumBinding<>();
        sequenceBinding.target = TypedOption.reference(() -> stochasticRelaxationState.sequence, v -> stochasticRelaxationState.sequence = v);
        sequenceBinding.values = sequenceValues;
        sequenceBinding.enumType = Sampler4DSequence.class;

        EnumBinding<StochasticRaytracingApproximation> approximationBinding = new EnumBinding<>();
        approximationBinding.target = TypedOption.reference(() -> stochasticRelaxationState.approximationOrderType, v -> stochasticRelaxationState.approximationOrderType = v);
        approximationBinding.values = approximateValues;
        approximationBinding.enumType = StochasticRaytracingApproximation.class;

        EnumBinding<RandomWalkEstimatorType> estimatorTypeBinding = new EnumBinding<>();
        estimatorTypeBinding.target = TypedOption.reference(() -> stochasticRelaxationState.randomWalkEstimatorType, v -> stochasticRelaxationState.randomWalkEstimatorType = v);
        estimatorTypeBinding.values = estimatorTypeValues;
        estimatorTypeBinding.enumType = RandomWalkEstimatorType.class;

        EnumBinding<RandomWalkEstimatorKind> estimatorKindBinding = new EnumBinding<>();
        estimatorKindBinding.target = TypedOption.reference(() -> stochasticRelaxationState.randomWalkEstimatorKind, v -> stochasticRelaxationState.randomWalkEstimatorKind = v);
        estimatorKindBinding.values = estimatorKindValues;
        estimatorKindBinding.enumType = RandomWalkEstimatorKind.class;

        TypedOption<Integer> rwrRayUnitsOpt = new TypedOption<>(
            "-rwr-ray-units",
            TypedOption.reference(() -> stochasticRelaxationState.rayUnitsPerIt, v -> stochasticRelaxationState.rayUnitsPerIt = v),
            1,
            null,
            null);
        TypedOption<Integer> rwrContinuousOpt = new TypedOption<>(
            "-rwr-continuous",
            TypedOption.reference(() -> stochasticRelaxationState.continuousRandomWalk, v -> stochasticRelaxationState.continuousRandomWalk = v),
            1,
            null,
            OptionsGroupRandomWalkRadiosity::parseBoolInt);
        TypedOption<Integer> rwrControlVariateOpt = new TypedOption<>(
            "-rwr-control-variate",
            TypedOption.reference(() -> stochasticRelaxationState.constantControlVariate, v -> stochasticRelaxationState.constantControlVariate = v),
            1,
            null,
            OptionsGroupRandomWalkRadiosity::parseBoolInt);
        TypedOption<Integer> rwrIndirectOnlyOpt = new TypedOption<>(
            "-rwr-indirect-only",
            TypedOption.reference(() -> stochasticRelaxationState.indirectOnly, v -> stochasticRelaxationState.indirectOnly = v),
            1,
            null,
            OptionsGroupRandomWalkRadiosity::parseBoolInt);
        TypedOption<EnumBinding<Sampler4DSequence>> rwrSequenceOpt = new TypedOption<>(
            "-rwr-sampling-sequence",
            TypedOption.valueRef(sequenceBinding),
            1,
            null,
            OptionsGroupRandomWalkRadiosity::parseEnumBinding);
        TypedOption<EnumBinding<StochasticRaytracingApproximation>> rwrApproximationOpt = new TypedOption<>(
            "-rwr-approximation",
            TypedOption.valueRef(approximationBinding),
            1,
            null,
            OptionsGroupRandomWalkRadiosity::parseEnumBinding);
        TypedOption<EnumBinding<RandomWalkEstimatorType>> rwrEstimatorOpt = new TypedOption<>(
            "-rwr-estimator",
            TypedOption.valueRef(estimatorTypeBinding),
            1,
            null,
            OptionsGroupRandomWalkRadiosity::parseEnumBinding);
        TypedOption<EnumBinding<RandomWalkEstimatorKind>> rwrScoreOpt = new TypedOption<>(
            "-rwr-score",
            TypedOption.valueRef(estimatorKindBinding),
            1,
            null,
            OptionsGroupRandomWalkRadiosity::parseEnumBinding);
        TypedOption<Integer> rwrNumlastOpt = new TypedOption<>(
            "-rwr-numlast",
            TypedOption.reference(() -> stochasticRelaxationState.randomWalkNumLast, v -> stochasticRelaxationState.randomWalkNumLast = v),
            1,
            null,
            null);
        OptionBase[] rwrOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(rwrRayUnitsOpt, 8),
            TypedOption.REGISTER_OPTION(rwrContinuousOpt, 7),
            TypedOption.REGISTER_OPTION(rwrControlVariateOpt, 7),
            TypedOption.REGISTER_OPTION(rwrIndirectOnlyOpt, 7),
            TypedOption.REGISTER_OPTION(rwrSequenceOpt, 7),
            TypedOption.REGISTER_OPTION(rwrApproximationOpt, 7),
            TypedOption.REGISTER_OPTION(rwrEstimatorOpt, 7),
            TypedOption.REGISTER_OPTION(rwrScoreOpt, 7),
            TypedOption.REGISTER_OPTION(rwrNumlastOpt, 12)
        };
        OptionGroup[] rwrGroups = new OptionGroup[] {
            new OptionGroup("randomWalk", rwrOptions, 9)
        };
        OptionParser.parse(argc, argv, rwrGroups, 1);
    }
}
