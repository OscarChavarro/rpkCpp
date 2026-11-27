#include <strings.h>

#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/logging/LoggerFormat.h"
#include "vsdk/toolkit/galerkin/GalerkinRadianceMethod.h"
#include "vsdk/toolkit/common/commandLineOptions/OptionParser.h"
#include "vsdk/toolkit/common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupGalerkin.h"

int OptionsGroupGalerkin::trueValue = true;
int OptionsGroupGalerkin::falseValue = false;

void
OptionsGroupGalerkin::iterationMethodOption(char *&name) {
    if ( strncasecmp(name, "jacobi", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = JACOBI;
    } else if ( strncasecmp(name, "gaussseidel", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = GAUSS_SEIDEL;
    } else if ( strncasecmp(name, "southwell", 2) == 0 ) {
        GalerkinRadianceMethod::galerkinState.galerkinIterationMethod = SOUTH_WELL;
    } else {
        Logger::reportMessage("OptionsGroupGalerkin", Logger::ERROR, "", LoggerFormat::format("Invalid iteration method '%s'", name));
    }
}

void
OptionsGroupGalerkin::shaftCullingOption(char *&name) {
    if ( strcasecmp(name, "never") == 0 ) {
        GalerkinRadianceMethod::galerkinState.shaftCullMode = NEVER_DO_SHAFT_CULLING;
    } else if ( strcasecmp(name, "refinement") == 0 ) {
        GalerkinRadianceMethod::galerkinState.shaftCullMode = DO_SHAFT_CULLING_FOR_REFINEMENT;
    } else if ( strcasecmp(name, "always") == 0 ) {
        GalerkinRadianceMethod::galerkinState.shaftCullMode = ALWAYS_DO_SHAFT_CULLING;
    } else {
        Logger::reportMessage("OptionsGroupGalerkin", Logger::ERROR, "", LoggerFormat::format("Invalid shaft culling mode '%s' (never, refinement or always)", name));
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
    char *iterationMethodName = NULL;
    TypedOption<char *> iterationMethodOpt("-gr-iteration-method", &iterationMethodName, 1, OptionsGroupGalerkin::iterationMethodOption, NULL);
    char *shaftCullingName = NULL;
    TypedOption<char *> shaftCullingOpt("-gr-shaft-culling", &shaftCullingName, 1, OptionsGroupGalerkin::shaftCullingOption, NULL);
    TypedOption<int> grHierarchicalOpt("-gr-hierarchical", &trueValue, 0, OptionsGroupGalerkin::hierarchicalOption, NULL);
    TypedOption<int> grNotHierarchicalOpt("-gr-not-hierarchical", &falseValue, 0, OptionsGroupGalerkin::hierarchicalOption, NULL);
    TypedOption<int> grLazyOpt("-gr-lazy-linking", &trueValue, 0, OptionsGroupGalerkin::lazyOption, NULL);
    TypedOption<int> grNoLazyOpt("-gr-no-lazy-linking", &falseValue, 0, OptionsGroupGalerkin::lazyOption, NULL);
    TypedOption<int> grClusteringOpt("-gr-clustering", &trueValue, 0, OptionsGroupGalerkin::clusteringOption, NULL);
    TypedOption<int> grNoClusteringOpt("-gr-no-clustering", &falseValue, 0, OptionsGroupGalerkin::clusteringOption, NULL);
    TypedOption<int> grImportanceOpt("-gr-importance", &trueValue, 0, OptionsGroupGalerkin::importanceOption, NULL);
    TypedOption<int> grNoImportanceOpt("-gr-no-importance", &falseValue, 0, OptionsGroupGalerkin::importanceOption, NULL);
    TypedOption<int> grAmbientOpt("-gr-ambient", &trueValue, 0, OptionsGroupGalerkin::ambientOption, NULL);
    TypedOption<int> grNoAmbientOpt("-gr-no-ambient", &falseValue, 0, OptionsGroupGalerkin::ambientOption, NULL);
    TypedOption<double> grLinkErrorOpt("-gr-link-error-threshold", &GalerkinRadianceMethod::galerkinState.relLinkErrorThreshold, 1, NULL, NULL);
    TypedOption<double> grMinElemAreaOpt("-gr-min-elem-area", &GalerkinRadianceMethod::galerkinState.relMinElemArea, 1, NULL, NULL);
    OptionBase galerkinOptions[] = {
        REGISTER_OPTION(char *, iterationMethodOpt, 6),
        REGISTER_OPTION(char *, shaftCullingOpt, 6),
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
        REGISTER_OPTION(double, grLinkErrorOpt, 6),
        REGISTER_OPTION(double, grMinElemAreaOpt, 6)
    };
    OptionGroup galerkinGroups[] = {
        OptionGroup("galerkin", galerkinOptions, 14)
    };
    OptionParser<OptionBase>::parse(argc, argv, galerkinGroups, 1);
}
