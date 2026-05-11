#include <cstring>
#include <cstring>

#include "vsdk/toolkit/common/commandLineOptions/OptionParser.h"
#include "vsdk/toolkit/common/commandLineOptions/TypedOption.h"
#include "options/OptionsGroupRandomWalkRadiosity.h"

template<typename T>
bool OptionsGroupRandomWalkRadiosity::parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
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
OptionsGroupRandomWalkRadiosity::parseBoolInt(int argc, char **argv, int &value) {
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

EnumDesc OptionsGroupRandomWalkRadiosity::approximateValues[] = {
    {StochasticRaytracingApproximation::CONSTANT, "constant", 2},
    {StochasticRaytracingApproximation::LINEAR, "linear", 2},
    {StochasticRaytracingApproximation::BI_LINEAR, "bilinear", 2},
    {StochasticRaytracingApproximation::QUADRATIC, "quadratic", 2},
    {StochasticRaytracingApproximation::CUBIC, "cubic", 2},
    {0, nullptr, 0}
};

EnumDesc OptionsGroupRandomWalkRadiosity::sequenceValues[] = {
    {Sampler4DSequence::RANDOM, "PseudoRandom", 2},
    {Sampler4DSequence::HALTON, "Halton", 2},
    {Sampler4DSequence::NIEDERREITER, "Niederreiter", 2},
    {0, nullptr, 0}
};

EnumDesc OptionsGroupRandomWalkRadiosity::estimatorTypeValues[] = {
    {RandomWalkEstimatorType::RW_SHOOTING, "Shooting", 2},
    {RandomWalkEstimatorType::RW_GATHERING, "Gathering", 2},
    {0, nullptr, 0}
};

EnumDesc OptionsGroupRandomWalkRadiosity::estimatorKindValues[] = {
    {RandomWalkEstimatorKind::RW_COLLISION, "Collision", 2},
    {RandomWalkEstimatorKind::RW_ABSORPTION, "Absorption", 2},
    {RandomWalkEstimatorKind::RW_SURVIVAL, "Survival", 2},
    {RandomWalkEstimatorKind::RW_LAST_BUT_NTH, "Last-but-N", 2},
    {RandomWalkEstimatorKind::RW_N_LAST, "Last-N", 2},
    {0, nullptr, 0}
};

void
OptionsGroupRandomWalkRadiosity::parse(
        int *argc,
        char **argv,
        StochasticRelaxation &stochasticRelaxationState)
{
    EnumBinding<Sampler4DSequence> sequenceBinding = {&stochasticRelaxationState.sequence, sequenceValues};
    EnumBinding<StochasticRaytracingApproximation> approximationBinding = {&stochasticRelaxationState.approximationOrderType, approximateValues};
    EnumBinding<RandomWalkEstimatorType> estimatorTypeBinding = {&stochasticRelaxationState.randomWalkEstimatorType, estimatorTypeValues};
    EnumBinding<RandomWalkEstimatorKind> estimatorKindBinding = {&stochasticRelaxationState.randomWalkEstimatorKind, estimatorKindValues};
    TypedOption<int> rwrRayUnitsOpt = {"-rwr-ray-units", &stochasticRelaxationState.rayUnitsPerIt, 1, nullptr, nullptr};
    TypedOption<int> rwrContinuousOpt = {"-rwr-continuous", &stochasticRelaxationState.continuousRandomWalk, 1, nullptr, parseBoolInt};
    TypedOption<int> rwrControlVariateOpt = {"-rwr-control-variate", &stochasticRelaxationState.constantControlVariate, 1, nullptr, parseBoolInt};
    TypedOption<int> rwrIndirectOnlyOpt = {"-rwr-indirect-only", &stochasticRelaxationState.indirectOnly, 1, nullptr, parseBoolInt};
    TypedOption<EnumBinding<Sampler4DSequence>> rwrSequenceOpt = {"-rwr-sampling-sequence", &sequenceBinding, 1, nullptr, parseEnumBinding<Sampler4DSequence>};
    TypedOption<EnumBinding<StochasticRaytracingApproximation>> rwrApproximationOpt = {"-rwr-approximation", &approximationBinding, 1, nullptr, parseEnumBinding<StochasticRaytracingApproximation>};
    TypedOption<EnumBinding<RandomWalkEstimatorType>> rwrEstimatorOpt = {"-rwr-estimator", &estimatorTypeBinding, 1, nullptr, parseEnumBinding<RandomWalkEstimatorType>};
    TypedOption<EnumBinding<RandomWalkEstimatorKind>> rwrScoreOpt = {"-rwr-score", &estimatorKindBinding, 1, nullptr, parseEnumBinding<RandomWalkEstimatorKind>};
    TypedOption<int> rwrNumlastOpt = {"-rwr-numlast", &stochasticRelaxationState.randomWalkNumLast, 1, nullptr, nullptr};
    OptionBase rwrOptions[] = {
        REGISTER_OPTION(int, rwrRayUnitsOpt, 8),
        REGISTER_OPTION(int, rwrContinuousOpt, 7),
        REGISTER_OPTION(int, rwrControlVariateOpt, 7),
        REGISTER_OPTION(int, rwrIndirectOnlyOpt, 7),
        REGISTER_OPTION(EnumBinding<Sampler4DSequence>, rwrSequenceOpt, 7),
        REGISTER_OPTION(EnumBinding<StochasticRaytracingApproximation>, rwrApproximationOpt, 7),
        REGISTER_OPTION(EnumBinding<RandomWalkEstimatorType>, rwrEstimatorOpt, 7),
        REGISTER_OPTION(EnumBinding<RandomWalkEstimatorKind>, rwrScoreOpt, 7),
        REGISTER_OPTION(int, rwrNumlastOpt, 12)
    };
    OptionGroup rwrGroups[] = {
        OptionGroup("randomWalk", rwrOptions, 9)
    };
    OptionParser<OptionBase>::parse(argc, argv, rwrGroups, 1);
}
