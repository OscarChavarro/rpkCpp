#include <string.h>
#include <strings.h>

#include "vsdk/common/commandLineOptions/OptionParser.h"
#include "vsdk/common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupRandomWalkRadiosity.h"

template<typename T>
bool OptionsGroupRandomWalkRadiosity::parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
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
OptionsGroupRandomWalkRadiosity::parseBoolInt(int argc, char **argv, int &value) {
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

EnumDesc OptionsGroupRandomWalkRadiosity::approximateValues[] = {
    {CONSTANT, "constant", 2},
    {LINEAR, "linear", 2},
    {BI_LINEAR, "bilinear", 2},
    {QUADRATIC, "quadratic", 2},
    {CUBIC, "cubic", 2},
    {0, NULL, 0}
};

EnumDesc OptionsGroupRandomWalkRadiosity::sequenceValues[] = {
    {RANDOM, "PseudoRandom", 2},
    {HALTON, "Halton", 2},
    {NIEDERREITER, "Niederreiter", 2},
    {0, NULL, 0}
};

EnumDesc OptionsGroupRandomWalkRadiosity::estimatorTypeValues[] = {
    {RW_SHOOTING, "Shooting", 2},
    {RW_GATHERING, "Gathering", 2},
    {0, NULL, 0}
};

EnumDesc OptionsGroupRandomWalkRadiosity::estimatorKindValues[] = {
    {RW_COLLISION, "Collision", 2},
    {RW_ABSORPTION, "Absorption", 2},
    {RW_SURVIVAL, "Survival", 2},
    {RW_LAST_BUT_NTH, "Last-but-N", 2},
    {RW_N_LAST, "Last-N", 2},
    {0, NULL, 0}
};

void
OptionsGroupRandomWalkRadiosity::parse(
        int *argc,
        char **argv,
        StochasticRelaxation &stochasticRelaxationState)
{
    EnumBinding<Sampler4DSequence> sequenceBinding = {&stochasticRelaxationState.sequence, sequenceValues};
    EnumBinding<StochRaytrApprx> approximationBinding = {&stochasticRelaxationState.approximationOrderType, approximateValues};
    EnumBinding<RandomWalkEstimatorType> estimatorTypeBinding = {&stochasticRelaxationState.randomWalkEstimatorType, estimatorTypeValues};
    EnumBinding<RandomWalkEstimatorKind> estimatorKindBinding = {&stochasticRelaxationState.randomWalkEstimatorKind, estimatorKindValues};
    TypedOption<int> rwrRayUnitsOpt("-rwr-ray-units", &stochasticRelaxationState.rayUnitsPerIt, 1, NULL, NULL);
    TypedOption<int> rwrContinuousOpt("-rwr-continuous", &stochasticRelaxationState.continuousRandomWalk, 1, NULL, parseBoolInt);
    TypedOption<int> rwrControlVariateOpt("-rwr-control-variate", &stochasticRelaxationState.constantControlVariate, 1, NULL, parseBoolInt);
    TypedOption<int> rwrIndirectOnlyOpt("-rwr-indirect-only", &stochasticRelaxationState.indirectOnly, 1, NULL, parseBoolInt);
    TypedOption<EnumBinding<Sampler4DSequence> > rwrSequenceOpt("-rwr-sampling-sequence", &sequenceBinding, 1, NULL, parseEnumBinding<Sampler4DSequence>);
    TypedOption<EnumBinding<StochRaytrApprx> > rwrApproximationOpt("-rwr-approximation", &approximationBinding, 1, NULL, parseEnumBinding<StochRaytrApprx>);
    TypedOption<EnumBinding<RandomWalkEstimatorType> > rwrEstimatorOpt("-rwr-estimator", &estimatorTypeBinding, 1, NULL, parseEnumBinding<RandomWalkEstimatorType>);
    TypedOption<EnumBinding<RandomWalkEstimatorKind> > rwrScoreOpt("-rwr-score", &estimatorKindBinding, 1, NULL, parseEnumBinding<RandomWalkEstimatorKind>);
    TypedOption<int> rwrNumlastOpt("-rwr-numlast", &stochasticRelaxationState.randomWalkNumLast, 1, NULL, NULL);
    OptionBase rwrOptions[] = {
        REGISTER_OPTION(int, rwrRayUnitsOpt, 8),
        REGISTER_OPTION(int, rwrContinuousOpt, 7),
        REGISTER_OPTION(int, rwrControlVariateOpt, 7),
        REGISTER_OPTION(int, rwrIndirectOnlyOpt, 7),
        REGISTER_OPTION(EnumBinding<Sampler4DSequence>, rwrSequenceOpt, 7),
        REGISTER_OPTION(EnumBinding<StochRaytrApprx>, rwrApproximationOpt, 7),
        REGISTER_OPTION(EnumBinding<RandomWalkEstimatorType>, rwrEstimatorOpt, 7),
        REGISTER_OPTION(EnumBinding<RandomWalkEstimatorKind>, rwrScoreOpt, 7),
        REGISTER_OPTION(int, rwrNumlastOpt, 12)
    };
    OptionGroup rwrGroups[] = {
        OptionGroup("randomWalk", rwrOptions, 9)
    };
    OptionParser<OptionBase>::parse(argc, argv, rwrGroups, 1);
}
