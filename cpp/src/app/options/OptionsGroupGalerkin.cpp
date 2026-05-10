#include <cstring>

#include "common/logging/Logger.h"
#include "galerkin/GalerkinRadianceMethod.h"
#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupGalerkin.h"

int OptionsGroupGalerkin::trueValue = true;
int OptionsGroupGalerkin::falseValue = false;

void
OptionsGroupGalerkin::iterationMethodOption(char *&name) {
    if ( strncasecmp(name, "jacobi", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = GalerkinIterationMethod::JACOBI;
    } else if ( strncasecmp(name, "gaussseidel", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = GalerkinIterationMethod::GAUSS_SEIDEL;
    } else if ( strncasecmp(name, "southwell", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = GalerkinIterationMethod::SOUTH_WELL;
    } else {
        Logger::error(nullptr, "Invalid iteration method '%s'", name);
    }
}

void
OptionsGroupGalerkin::hierarchicalOption(int &yesno) {
    if ( yesno != 0 ) {
        GalerkinRadianceMethod::galerkinState.hierarchical = true;
    } else {
        GalerkinRadianceMethod::galerkinState.hierarchical = false;
    }
}

void
OptionsGroupGalerkin::lazyOption(int &yesno) {
    GalerkinRadianceMethod::galerkinState.lazyLinking = yesno;
}

void
OptionsGroupGalerkin::clusteringOption(int &yesno) {
    GalerkinRadianceMethod::galerkinState.clustered = yesno;
}

void
OptionsGroupGalerkin::importanceOption(int &yesno) {
    GalerkinRadianceMethod::galerkinState.importanceDriven = yesno;
}

void
OptionsGroupGalerkin::ambientOption(int &yesno) {
    GalerkinRadianceMethod::galerkinState.useAmbientRadiance = yesno;
}

void
OptionsGroupGalerkin::galerkinParseOptions(int *argc, char **argv) {
    char *iterationMethodName = nullptr;
    TypedOption<char *> iterationMethodOpt = {"-gr-iteration-method", &iterationMethodName, 1, OptionsGroupGalerkin::iterationMethodOption, nullptr};
    TypedOption<int> grHierarchicalOpt = {"-gr-hierarchical", &trueValue, 0, OptionsGroupGalerkin::hierarchicalOption, nullptr};
    TypedOption<int> grNotHierarchicalOpt = {"-gr-not-hierarchical", &falseValue, 0, OptionsGroupGalerkin::hierarchicalOption, nullptr};
    TypedOption<int> grLazyOpt = {"-gr-lazy-linking", &trueValue, 0, OptionsGroupGalerkin::lazyOption, nullptr};
    TypedOption<int> grNoLazyOpt = {"-gr-no-lazy-linking", &falseValue, 0, OptionsGroupGalerkin::lazyOption, nullptr};
    TypedOption<int> grClusteringOpt = {"-gr-clustering", &trueValue, 0, OptionsGroupGalerkin::clusteringOption, nullptr};
    TypedOption<int> grNoClusteringOpt = {"-gr-no-clustering", &falseValue, 0, OptionsGroupGalerkin::clusteringOption, nullptr};
    TypedOption<int> grImportanceOpt = {"-gr-importance", &trueValue, 0, OptionsGroupGalerkin::importanceOption, nullptr};
    TypedOption<int> grNoImportanceOpt = {"-gr-no-importance", &falseValue, 0, OptionsGroupGalerkin::importanceOption, nullptr};
    TypedOption<int> grAmbientOpt = {"-gr-ambient", &trueValue, 0, OptionsGroupGalerkin::ambientOption, nullptr};
    TypedOption<int> grNoAmbientOpt = {"-gr-no-ambient", &falseValue, 0, OptionsGroupGalerkin::ambientOption, nullptr};
    TypedOption<float> grLinkErrorOpt = {"-gr-link-error-threshold", &GalerkinRadianceMethod::galerkinState.relLinkErrorThreshold, 1, nullptr, nullptr};
    TypedOption<float> grMinElemAreaOpt = {"-gr-min-elem-area", &GalerkinRadianceMethod::galerkinState.relMinElemArea, 1, nullptr, nullptr};
    OptionBase galerkinOptions[] = {
        REGISTER_OPTION(char *, iterationMethodOpt, 6),
        REGISTER_OPTION(int, grHierarchicalOpt, 6),
        REGISTER_OPTION(int, grNotHierarchicalOpt, 10),
        REGISTER_OPTION(int, grLazyOpt, 6),
        REGISTER_OPTION(int, grNoLazyOpt, 10),
        REGISTER_OPTION(int, grClusteringOpt, 6),
        REGISTER_OPTION(int, grNoClusteringOpt, 10),
        REGISTER_OPTION(int, grImportanceOpt, 6),
        REGISTER_OPTION(int, grNoImportanceOpt, 10),
        REGISTER_OPTION(int, grAmbientOpt, 6),
        REGISTER_OPTION(int, grNoAmbientOpt, 10),
        REGISTER_OPTION(float, grLinkErrorOpt, 6),
        REGISTER_OPTION(float, grMinElemAreaOpt, 6)
    };
    OptionGroup galerkinGroups[] = {
        OptionGroup("galerkin", galerkinOptions, 13)
    };
    OptionParser<OptionBase>::parse(argc, argv, galerkinGroups, 1);
}
