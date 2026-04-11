import { Error as VsdkError } from "../../common/Error";
import { OptionBase } from "../../common/commandLineOptions/OptionBase";
import { OptionGroup } from "../../common/commandLineOptions/OptionGroup";
import { OptionParser } from "../../common/commandLineOptions/OptionParser";
import { TypedOption } from "../../common/commandLineOptions/TypedOption";
import { GalerkinIterationMethod } from "../../galerkin/GalerkinIterationMethod";
import { GalerkinRadianceMethod } from "../../galerkin/GalerkinRadianceMethod";
import { OptionTextUtils } from "./OptionTextUtils";

export class OptionsGroupGalerkin {
  private static trueValue = 1;
  private static falseValue = 0;

  private constructor() {
  }

  private static iterationMethodOption(value: TypedOption.MutableValue<string>): void {
    const name = value.value;
    if (OptionTextUtils.equalsIgnoreCasePrefix(name, "jacobi", 2)) {
      GalerkinRadianceMethod.galerkinState.galerkinIterationMethod = GalerkinIterationMethod.JACOBI;
    }
    else if (OptionTextUtils.equalsIgnoreCasePrefix(name, "gaussseidel", 2)) {
      GalerkinRadianceMethod.galerkinState.galerkinIterationMethod = GalerkinIterationMethod.GAUSS_SEIDEL;
    }
    else if (OptionTextUtils.equalsIgnoreCasePrefix(name, "southwell", 2)) {
      GalerkinRadianceMethod.galerkinState.galerkinIterationMethod = GalerkinIterationMethod.SOUTH_WELL;
    }
    else {
      VsdkError.error(null, "Invalid iteration method '%s'", name);
    }
  }

  private static hierarchicalOption(value: TypedOption.MutableValue<number>): void {
    GalerkinRadianceMethod.galerkinState.hierarchical = value.value !== 0;
  }

  private static lazyOption(value: TypedOption.MutableValue<number>): void {
    GalerkinRadianceMethod.galerkinState.lazyLinking = value.value;
  }

  private static clusteringOption(value: TypedOption.MutableValue<number>): void {
    GalerkinRadianceMethod.galerkinState.clustered = value.value;
  }

  private static importanceOption(value: TypedOption.MutableValue<number>): void {
    GalerkinRadianceMethod.galerkinState.importanceDriven = value.value;
  }

  private static ambientOption(value: TypedOption.MutableValue<number>): void {
    GalerkinRadianceMethod.galerkinState.useAmbientRadiance = value.value;
  }

  public static galerkinParseOptions(argc: number[], argv: string[]): void {
    const iterationMethodName = TypedOption.valueRef<string | null>(null);

    const iterationMethodOpt = new TypedOption<string | null>(
      "-gr-iteration-method",
      iterationMethodName,
      1,
      (v) => OptionsGroupGalerkin.iterationMethodOption(v as TypedOption.MutableValue<string>),
      null
    );
    const grHierarchicalOpt = new TypedOption<number>(
      "-gr-hierarchical",
      TypedOption.valueRef(OptionsGroupGalerkin.trueValue),
      0,
      OptionsGroupGalerkin.hierarchicalOption,
      null
    );
    const grNotHierarchicalOpt = new TypedOption<number>(
      "-gr-not-hierarchical",
      TypedOption.valueRef(OptionsGroupGalerkin.falseValue),
      0,
      OptionsGroupGalerkin.hierarchicalOption,
      null
    );
    const grLazyOpt = new TypedOption<number>(
      "-gr-lazy-linking",
      TypedOption.valueRef(OptionsGroupGalerkin.trueValue),
      0,
      OptionsGroupGalerkin.lazyOption,
      null
    );
    const grNoLazyOpt = new TypedOption<number>(
      "-gr-no-lazy-linking",
      TypedOption.valueRef(OptionsGroupGalerkin.falseValue),
      0,
      OptionsGroupGalerkin.lazyOption,
      null
    );
    const grClusteringOpt = new TypedOption<number>(
      "-gr-clustering",
      TypedOption.valueRef(OptionsGroupGalerkin.trueValue),
      0,
      OptionsGroupGalerkin.clusteringOption,
      null
    );
    const grNoClusteringOpt = new TypedOption<number>(
      "-gr-no-clustering",
      TypedOption.valueRef(OptionsGroupGalerkin.falseValue),
      0,
      OptionsGroupGalerkin.clusteringOption,
      null
    );
    const grImportanceOpt = new TypedOption<number>(
      "-gr-importance",
      TypedOption.valueRef(OptionsGroupGalerkin.trueValue),
      0,
      OptionsGroupGalerkin.importanceOption,
      null
    );
    const grNoImportanceOpt = new TypedOption<number>(
      "-gr-no-importance",
      TypedOption.valueRef(OptionsGroupGalerkin.falseValue),
      0,
      OptionsGroupGalerkin.importanceOption,
      null
    );
    const grAmbientOpt = new TypedOption<number>(
      "-gr-ambient",
      TypedOption.valueRef(OptionsGroupGalerkin.trueValue),
      0,
      OptionsGroupGalerkin.ambientOption,
      null
    );
    const grNoAmbientOpt = new TypedOption<number>(
      "-gr-no-ambient",
      TypedOption.valueRef(OptionsGroupGalerkin.falseValue),
      0,
      OptionsGroupGalerkin.ambientOption,
      null
    );
    const grLinkErrorOpt = new TypedOption<number>(
      "-gr-link-error-threshold",
      TypedOption.reference(
        () => GalerkinRadianceMethod.galerkinState.relLinkErrorThreshold,
        (v) => {
          GalerkinRadianceMethod.galerkinState.relLinkErrorThreshold = v;
        }
      ),
      1,
      null,
      null
    );
    const grMinElemAreaOpt = new TypedOption<number>(
      "-gr-min-elem-area",
      TypedOption.reference(
        () => GalerkinRadianceMethod.galerkinState.relMinElemArea,
        (v) => {
          GalerkinRadianceMethod.galerkinState.relMinElemArea = v;
        }
      ),
      1,
      null,
      null
    );
    const galerkinOptions: OptionBase[] = [
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
      TypedOption.REGISTER_OPTION(grMinElemAreaOpt, 6),
    ];
    const galerkinGroups = [
      new OptionGroup("galerkin", galerkinOptions, 13),
    ];
    OptionParser.parse(argc, argv, galerkinGroups, 1);
  }
}
