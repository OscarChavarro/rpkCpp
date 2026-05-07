#include <cstring>
#include <cstring>

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"
#include "app/options/OptionsGroupStochasticRelaxationRadiosity.h"

template<typename T>
bool OptionsGroupStochasticRelaxationRadiosity::parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
    if ( argc < 1 || argv == nullptr || argv[0] == nullptr || binding.target == nullptr || binding.values == nullptr ) {
        return false;
    }
    for ( int i = 0; binding.values[i].name != nullptr; i++ ) {
        if ( strncasecmp(argv[0], binding.values[i].name, binding.values[i].abbrev) == 0 ) {
            *binding.target = static_cast<T>(binding.values[i].value);
            return true;
        }
    }
    return false;
}

bool
OptionsGroupStochasticRelaxationRadiosity::parseBoolInt(int argc, char **argv, int &value) {
    if ( argc < 1 || argv == nullptr || argv[0] == nullptr ) {
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

EnumDesc OptionsGroupStochasticRelaxationRadiosity::approximateValues[] = {
    {StochasticRaytracingApproximation::CONSTANT, "constant", 2},
    {StochasticRaytracingApproximation::LINEAR, "linear", 2},
    {StochasticRaytracingApproximation::BI_LINEAR, "bilinear", 2},
    {StochasticRaytracingApproximation::QUADRATIC, "quadratic", 2},
    {StochasticRaytracingApproximation::CUBIC, "cubic", 2},
    {0, nullptr, 0}
};

EnumDesc OptionsGroupStochasticRelaxationRadiosity::clusteringValues[] = {
    {HierarchyClusteringMode::NO_CLUSTERING, "none", 2},
    {HierarchyClusteringMode::ISOTROPIC_CLUSTERING, "isotropic", 2},
    {HierarchyClusteringMode::ORIENTED_CLUSTERING, "oriented", 2},
    {0, nullptr, 0}
};

EnumDesc OptionsGroupStochasticRelaxationRadiosity::sequenceValues[] = {
    {Sampler4DSequence::RANDOM, "PseudoRandom", 2},
    {Sampler4DSequence::HALTON, "Halton", 2},
    {Sampler4DSequence::NIEDERREITER, "Niederreiter", 2},
    {0, nullptr, 0}
};

EnumDesc OptionsGroupStochasticRelaxationRadiosity::showWhatValues[] = {
    {WhatToShow::SHOW_TOTAL_RADIANCE, "total-radiance", 2},
    {WhatToShow::SHOW_INDIRECT_RADIANCE, "indirect-radiance", 2},
    {WhatToShow::SHOW_IMPORTANCE, "importance", 2},
    {0, nullptr, 0}
};

void
OptionsGroupStochasticRelaxationRadiosity::parse(
        int *argc,
        char **argv,
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState)
{
    EnumBinding<Sampler4DSequence> sequenceBinding = {&stochasticRelaxationState.sequence, sequenceValues};
    EnumBinding<StochasticRaytracingApproximation> approximationBinding = {&stochasticRelaxationState.approximationOrderType, approximateValues};
    EnumBinding<HierarchyClusteringMode> clusteringBinding = {&elementHierarchyState.clustering, clusteringValues};
    EnumBinding<WhatToShow> showBinding = {&stochasticRelaxationState.show, showWhatValues};
    TypedOption<int> srrRayUnitsOpt = {"-srr-ray-units", &stochasticRelaxationState.rayUnitsPerIt, 1, nullptr, nullptr};
    TypedOption<int> srrBidirectionalOpt = {"-srr-bidirectional", &stochasticRelaxationState.bidirectionalTransfers, 1, nullptr, parseBoolInt};
    TypedOption<int> srrControlVariateOpt = {"-srr-control-variate", &stochasticRelaxationState.constantControlVariate, 1, nullptr, parseBoolInt};
    TypedOption<int> srrIndirectOnlyOpt = {"-srr-indirect-only", &stochasticRelaxationState.indirectOnly, 1, nullptr, parseBoolInt};
    TypedOption<int> srrImportanceDrivenOpt = {"-srr-importance-driven", &stochasticRelaxationState.importanceDriven, 1, nullptr, parseBoolInt};
    TypedOption<EnumBinding<Sampler4DSequence>> srrSequenceOpt = {"-srr-sampling-sequence", &sequenceBinding, 1, nullptr, parseEnumBinding<Sampler4DSequence>};
    TypedOption<EnumBinding<StochasticRaytracingApproximation>> srrApproximationOpt = {"-srr-approximation", &approximationBinding, 1, nullptr, parseEnumBinding<StochasticRaytracingApproximation>};
    TypedOption<int> srrHierarchicalOpt = {"-srr-hierarchical", &elementHierarchyState.do_h_meshing, 1, nullptr, parseBoolInt};
    TypedOption<EnumBinding<HierarchyClusteringMode>> srrClusteringOpt = {"-srr-clustering", &clusteringBinding, 1, nullptr, parseEnumBinding<HierarchyClusteringMode>};
    TypedOption<float> srrEpsilonOpt = {"-srr-epsilon", &elementHierarchyState.epsilon, 1, nullptr, nullptr};
    TypedOption<float> srrMinAreaOpt = {"-srr-minarea", &elementHierarchyState.minimumArea, 1, nullptr, nullptr};
    TypedOption<EnumBinding<WhatToShow>> srrDisplayOpt = {"-srr-display", &showBinding, 1, nullptr, parseEnumBinding<WhatToShow>};
    TypedOption<int> srrDiscardIncrementalOpt = {"-srr-discard-incremental", &stochasticRelaxationState.discardIncremental, 1, nullptr, parseBoolInt};
    TypedOption<int> srrIncrementalImportanceOpt = {"-srr-incremental-uses-importance", &stochasticRelaxationState.incrementalUsesImportance, 1, nullptr, parseBoolInt};
    TypedOption<int> srrNaiveMergingOpt = {"-srr-naive-merging", &stochasticRelaxationState.naiveMerging, 1, nullptr, parseBoolInt};
    TypedOption<int> srrNonDiffuseFirstShotOpt = {"-srr-nondiffuse-first-shot", &stochasticRelaxationState.doNonDiffuseFirstShot, 1, nullptr, parseBoolInt};
    TypedOption<int> srrInitialLsSamplesOpt = {"-srr-initial-ls-samples", &stochasticRelaxationState.initialLightSourceSamples, 1, nullptr, nullptr};
    OptionBase srrOptions[] = {
        REGISTER_OPTION(int, srrRayUnitsOpt, 8),
        REGISTER_OPTION(int, srrBidirectionalOpt, 7),
        REGISTER_OPTION(int, srrControlVariateOpt, 7),
        REGISTER_OPTION(int, srrIndirectOnlyOpt, 7),
        REGISTER_OPTION(int, srrImportanceDrivenOpt, 7),
        REGISTER_OPTION(EnumBinding<Sampler4DSequence>, srrSequenceOpt, 7),
        REGISTER_OPTION(EnumBinding<StochasticRaytracingApproximation>, srrApproximationOpt, 7),
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
