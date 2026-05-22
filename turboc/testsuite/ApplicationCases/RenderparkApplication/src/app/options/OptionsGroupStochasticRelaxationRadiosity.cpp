#include <string.h>
#include <strings.h>

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"
#include "app/options/OptionsGroupStochasticRelaxationRadiosity.h"

template<typename T>
bool OptsGrpStochRelaxRad::parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
    if ( argc < 1 || argv == NULL || argv[0] == NULL || binding.target == NULL || binding.values == NULL ) {
        return false;
    }
    for ( int i = 0; binding.values[i].name != NULL; i++ ) {
        if ( strncasecmp(argv[0], binding.values[i].name, binding.values[i].abbrev) == 0 ) {
            *binding.target = ((T)(binding.values[i].value));
            return true;
        }
    }
    return false;
}

bool
OptsGrpStochRelaxRad::parseSequenceBinding(int argc, char **argv, EnumBinding<Sampler4DSequence> &binding) {
    return parseEnumBinding<Sampler4DSequence>(argc, argv, binding);
}

bool
OptsGrpStochRelaxRad::parseApproximationBinding(int argc, char **argv, EnumBinding<StochRaytrApprx> &binding) {
    return parseEnumBinding<StochRaytrApprx>(argc, argv, binding);
}

bool
OptsGrpStochRelaxRad::parseClusteringBinding(int argc, char **argv, EnumBinding<HierarchyClusteringMode> &binding) {
    return parseEnumBinding<HierarchyClusteringMode>(argc, argv, binding);
}

bool
OptsGrpStochRelaxRad::parseShowBinding(int argc, char **argv, EnumBinding<WhatToShow> &binding) {
    return parseEnumBinding<WhatToShow>(argc, argv, binding);
}

bool
OptsGrpStochRelaxRad::parseBoolInt(int argc, char **argv, int &value) {
    if ( argc < 1 || argv == NULL || argv[0] == NULL ) {
        return false;
    }
    if ( strcasecmp(argv[0], "true") == 0 || strcasecmp(argv[0], "yes") == 0 || strcmp(argv[0], "1") == 0 ) {
        value = 1;
        return true;
    }
    if ( strcasecmp(argv[0], "false") == 0 || strcasecmp(argv[0], "no") == 0 || strcmp(argv[0], "0") == 0 ) {
        value = 0;
        return true;
    }
    return false;
}

EnumDesc OptsGrpStochRelaxRad::approximateValues[] = {
    {CONSTANT, "constant", 2},
    {LINEAR, "linear", 2},
    {BI_LINEAR, "bilinear", 2},
    {QUADRATIC, "quadratic", 2},
    {CUBIC, "cubic", 2},
    {0, NULL, 0}
};

EnumDesc OptsGrpStochRelaxRad::clusteringValues[] = {
    {NO_CLUSTERING, "none", 2},
    {ISOTROPIC_CLUSTERING, "isotropic", 2},
    {ORIENTED_CLUSTERING, "oriented", 2},
    {0, NULL, 0}
};

EnumDesc OptsGrpStochRelaxRad::sequenceValues[] = {
    {RANDOM, "PseudoRandom", 2},
    {HALTON, "Halton", 2},
    {NIEDERREITER, "Niederreiter", 2},
    {0, NULL, 0}
};

EnumDesc OptsGrpStochRelaxRad::showWhatValues[] = {
    {SHOW_TOTAL_RADIANCE, "total-radiance", 2},
    {SHOW_INDIRECT_RADIANCE, "indirect-radiance", 2},
    {SHOW_IMPORTANCE, "importance", 2},
    {0, NULL, 0}
};

void
OptsGrpStochRelaxRad::parse(
        int *argc,
        char **argv,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState)
{
    EnumBinding<Sampler4DSequence> sequenceBinding = {&stochasticRelaxationState.sequence, sequenceValues};
    EnumBinding<StochRaytrApprx> approximationBinding = {&stochasticRelaxationState.approximationOrderType, approximateValues};
    EnumBinding<HierarchyClusteringMode> clusteringBinding = {&elementHierarchyState.clustering, clusteringValues};
    EnumBinding<WhatToShow> showBinding = {&stochasticRelaxationState.show, showWhatValues};
    TypedOption<int> srrRayUnitsOpt("-srr-ray-units", &stochasticRelaxationState.rayUnitsPerIt, 1, NULL, NULL);
    TypedOption<int> srrBidirectionalOpt("-srr-bidirectional", &stochasticRelaxationState.bidirectionalTransfers, 1, NULL, parseBoolInt);
    TypedOption<int> srrControlVariateOpt("-srr-control-variate", &stochasticRelaxationState.constantControlVariate, 1, NULL, parseBoolInt);
    TypedOption<int> srrIndirectOnlyOpt("-srr-indirect-only", &stochasticRelaxationState.indirectOnly, 1, NULL, parseBoolInt);
    TypedOption<int> srrImportanceDrivenOpt("-srr-importance-driven", &stochasticRelaxationState.importanceDriven, 1, NULL, parseBoolInt);
    TypedOption<EnumBinding<Sampler4DSequence> > srrSequenceOpt("-srr-sampling-sequence", &sequenceBinding, 1, NULL, parseSequenceBinding);
    TypedOption<EnumBinding<StochRaytrApprx> > srrApproximationOpt("-srr-approximation", &approximationBinding, 1, NULL, parseApproximationBinding);
    TypedOption<int> srrHierarchicalOpt("-srr-hierarchical", &elementHierarchyState.do_h_meshing, 1, NULL, parseBoolInt);
    TypedOption<EnumBinding<HierarchyClusteringMode> > srrClusteringOpt("-srr-clustering", &clusteringBinding, 1, NULL, parseClusteringBinding);
    TypedOption<float> srrEpsilonOpt("-srr-epsilon", &elementHierarchyState.epsilon, 1, NULL, NULL);
    TypedOption<float> srrMinAreaOpt("-srr-minarea", &elementHierarchyState.minimumArea, 1, NULL, NULL);
    TypedOption<EnumBinding<WhatToShow> > srrDisplayOpt("-srr-display", &showBinding, 1, NULL, parseShowBinding);
    TypedOption<int> srrDiscardIncrementalOpt("-srr-discard-incremental", &stochasticRelaxationState.discardIncremental, 1, NULL, parseBoolInt);
    TypedOption<int> srrIncrementalImportanceOpt("-srr-incremental-uses-importance", &stochasticRelaxationState.incrementalUsesImportance, 1, NULL, parseBoolInt);
    TypedOption<int> srrNaiveMergingOpt("-srr-naive-merging", &stochasticRelaxationState.naiveMerging, 1, NULL, parseBoolInt);
    TypedOption<int> srrNonDiffuseFirstShotOpt("-srr-nondiffuse-first-shot", &stochasticRelaxationState.doNonDiffuseFirstShot, 1, NULL, parseBoolInt);
    TypedOption<int> srrInitialLsSamplesOpt("-srr-initial-ls-samples", &stochasticRelaxationState.initialLightSourceSamples, 1, NULL, NULL);
    OptionBase srrOptions[] = {
        REGISTER_OPTION(int, srrRayUnitsOpt, 8),
        REGISTER_OPTION(int, srrBidirectionalOpt, 7),
        REGISTER_OPTION(int, srrControlVariateOpt, 7),
        REGISTER_OPTION(int, srrIndirectOnlyOpt, 7),
        REGISTER_OPTION(int, srrImportanceDrivenOpt, 7),
        REGISTER_OPTION(EnumBinding<Sampler4DSequence>, srrSequenceOpt, 7),
        REGISTER_OPTION(EnumBinding<StochRaytrApprx>, srrApproximationOpt, 7),
        REGISTER_OPTION(int, srrHierarchicalOpt, 7),
        REGISTER_OPTION(EnumBinding<HierarchyClusteringMode>, srrClusteringOpt, 7),
        REGISTER_OPTION(float, srrEpsilonOpt, 7),
        REGISTER_OPTION(float, srrMinAreaOpt, 7),
        REGISTER_OPTION(EnumBinding<WhatToShow>, srrDisplayOpt, 7),
        REGISTER_OPTION(int, srrDiscardIncrementalOpt, 7),
        REGISTER_OPTION(int, srrIncrementalImportanceOpt, 7),
        REGISTER_OPTION(int, srrNaiveMergingOpt, 7),
        REGISTER_OPTION(int, srrNonDiffuseFirstShotOpt, 7),
        REGISTER_OPTION(int, srrInitialLsSamplesOpt, 7)
    };
    OptionGroup srrGroups[] = {
        OptionGroup("stochasticRelaxation", srrOptions, 17)
    };
    OptionParser<OptionBase>::parse(argc, argv, srrGroups, 1);
}
