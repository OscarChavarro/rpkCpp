package vsdk.toolkit.app.options;

import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.commandLineOptions.OptionBase;
import vsdk.toolkit.common.commandLineOptions.OptionGroup;
import vsdk.toolkit.common.commandLineOptions.OptionParser;
import vsdk.toolkit.common.commandLineOptions.TypedOption;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinRadianceMethod;

public final class OptionsGroupGalerkin {
    private static int trueValue = 1;
    private static int falseValue = 0;

    private OptionsGroupGalerkin() {
    }

    private static void iterationMethodOption(TypedOption.MutableValue<String> value) {
        String name = value.value;
        if ( OptionTextUtils.equalsIgnoreCasePrefix(name, "jacobi", 2) ) {
            GalerkinRadianceMethod.galerkinState.galerkinIterationMethod = GalerkinIterationMethod.JACOBI;
        }
        else if ( OptionTextUtils.equalsIgnoreCasePrefix(name, "gaussseidel", 2) ) {
            GalerkinRadianceMethod.galerkinState.galerkinIterationMethod = GalerkinIterationMethod.GAUSS_SEIDEL;
        }
        else if ( OptionTextUtils.equalsIgnoreCasePrefix(name, "southwell", 2) ) {
            GalerkinRadianceMethod.galerkinState.galerkinIterationMethod = GalerkinIterationMethod.SOUTH_WELL;
        }
        else {
            Error.error(null, "Invalid iteration method '%s'", name);
        }
    }

    private static void hierarchicalOption(TypedOption.MutableValue<Integer> value) {
        GalerkinRadianceMethod.galerkinState.hierarchical = value.value != 0;
    }

    private static void lazyOption(TypedOption.MutableValue<Integer> value) {
        GalerkinRadianceMethod.galerkinState.lazyLinking = value.value;
    }

    private static void clusteringOption(TypedOption.MutableValue<Integer> value) {
        GalerkinRadianceMethod.galerkinState.clustered = value.value;
    }

    private static void importanceOption(TypedOption.MutableValue<Integer> value) {
        GalerkinRadianceMethod.galerkinState.importanceDriven = value.value;
    }

    private static void ambientOption(TypedOption.MutableValue<Integer> value) {
        GalerkinRadianceMethod.galerkinState.useAmbientRadiance = value.value;
    }

    public static void galerkinParseOptions(int[] argc, String[] argv) {
        TypedOption.ValueRef<String> iterationMethodName = TypedOption.valueRef((String)null);

        TypedOption<String> iterationMethodOpt = new TypedOption<>(
            "-gr-iteration-method",
            iterationMethodName,
            1,
            OptionsGroupGalerkin::iterationMethodOption,
            null);
        TypedOption<Integer> grHierarchicalOpt = new TypedOption<>(
            "-gr-hierarchical",
            TypedOption.valueRef(trueValue),
            0,
            OptionsGroupGalerkin::hierarchicalOption,
            null);
        TypedOption<Integer> grNotHierarchicalOpt = new TypedOption<>(
            "-gr-not-hierarchical",
            TypedOption.valueRef(falseValue),
            0,
            OptionsGroupGalerkin::hierarchicalOption,
            null);
        TypedOption<Integer> grLazyOpt = new TypedOption<>(
            "-gr-lazy-linking",
            TypedOption.valueRef(trueValue),
            0,
            OptionsGroupGalerkin::lazyOption,
            null);
        TypedOption<Integer> grNoLazyOpt = new TypedOption<>(
            "-gr-no-lazy-linking",
            TypedOption.valueRef(falseValue),
            0,
            OptionsGroupGalerkin::lazyOption,
            null);
        TypedOption<Integer> grClusteringOpt = new TypedOption<>(
            "-gr-clustering",
            TypedOption.valueRef(trueValue),
            0,
            OptionsGroupGalerkin::clusteringOption,
            null);
        TypedOption<Integer> grNoClusteringOpt = new TypedOption<>(
            "-gr-no-clustering",
            TypedOption.valueRef(falseValue),
            0,
            OptionsGroupGalerkin::clusteringOption,
            null);
        TypedOption<Integer> grImportanceOpt = new TypedOption<>(
            "-gr-importance",
            TypedOption.valueRef(trueValue),
            0,
            OptionsGroupGalerkin::importanceOption,
            null);
        TypedOption<Integer> grNoImportanceOpt = new TypedOption<>(
            "-gr-no-importance",
            TypedOption.valueRef(falseValue),
            0,
            OptionsGroupGalerkin::importanceOption,
            null);
        TypedOption<Integer> grAmbientOpt = new TypedOption<>(
            "-gr-ambient",
            TypedOption.valueRef(trueValue),
            0,
            OptionsGroupGalerkin::ambientOption,
            null);
        TypedOption<Integer> grNoAmbientOpt = new TypedOption<>(
            "-gr-no-ambient",
            TypedOption.valueRef(falseValue),
            0,
            OptionsGroupGalerkin::ambientOption,
            null);
        TypedOption<Float> grLinkErrorOpt = new TypedOption<>(
            "-gr-link-error-threshold",
            TypedOption.reference(
                () -> GalerkinRadianceMethod.galerkinState.relLinkErrorThreshold,
                v -> GalerkinRadianceMethod.galerkinState.relLinkErrorThreshold = v),
            1,
            null,
            null);
        TypedOption<Float> grMinElemAreaOpt = new TypedOption<>(
            "-gr-min-elem-area",
            TypedOption.reference(
                () -> GalerkinRadianceMethod.galerkinState.relMinElemArea,
                v -> GalerkinRadianceMethod.galerkinState.relMinElemArea = v),
            1,
            null,
            null);
        OptionBase[] galerkinOptions = new OptionBase[] {
            TypedOption.REGISTER_OPTION(iterationMethodOpt, 6),
            TypedOption.REGISTER_OPTION(grHierarchicalOpt, 6),
            TypedOption.REGISTER_OPTION(grNotHierarchicalOpt, 10),
            TypedOption.REGISTER_OPTION(grLazyOpt, 6),
            TypedOption.REGISTER_OPTION(grNoLazyOpt, 10),
            TypedOption.REGISTER_OPTION(grClusteringOpt, 6),
            TypedOption.REGISTER_OPTION(grNoClusteringOpt, 10),
            TypedOption.REGISTER_OPTION(grImportanceOpt, 6),
            TypedOption.REGISTER_OPTION(grNoImportanceOpt, 10),
            TypedOption.REGISTER_OPTION(grAmbientOpt, 6),
            TypedOption.REGISTER_OPTION(grNoAmbientOpt, 10),
            TypedOption.REGISTER_OPTION(grLinkErrorOpt, 6),
            TypedOption.REGISTER_OPTION(grMinElemAreaOpt, 6)
        };
        OptionGroup[] galerkinGroups = new OptionGroup[] {
            new OptionGroup("galerkin", galerkinOptions, 13)
        };
        OptionParser.parse(argc, argv, galerkinGroups, 1);
    }
}
