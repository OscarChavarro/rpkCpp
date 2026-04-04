#include <cstdlib>
#include <cstring>
#include <strings.h>

#include "java/lang/System.h"
#include "common/Error.h"

#ifdef RAYTRACING_ENABLED
    #include "raycasting/simple/RayMatterState.h"
    #include "raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"
    #include "raycasting/stochasticRaytracing/Hierarchy.h"
    #include "raycasting/stochasticRaytracing/StochasticRayTracingState.h"
    #include "raycasting/stochasticRaytracing/StochasticRelaxation.h"
    #include "raycasting/photonMap/PhotonMapState.h"
#endif

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/CommandLine.h"

template<typename T>
bool CommandLine::parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
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
CommandLine::parseFixedStringBinding(int argc, char **argv, FixedStringBinding &binding) {
    if ( argc < 1 || argv == nullptr || argv[0] == nullptr || binding.target == nullptr || binding.maxLength <= 0 ) {
        return false;
    }
    strncpy(binding.target, argv[0], binding.maxLength);
    binding.target[binding.maxLength - 1] = '\0';
    return true;
}

bool
CommandLine::parseBoolInt(int argc, char **argv, int &value) {
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

void
CommandLine::setIntTrue(int &value) {
    value = 1;
}

void
CommandLine::setIntFalse(int &value) {
    value = 0;
}

#ifdef RAYTRACING_ENABLED

EnumDesc CommandLine::approximateValues[] = {
    {StochasticRaytracingApproximation::CONSTANT, "constant", 2},
    {StochasticRaytracingApproximation::LINEAR, "linear", 2},
    {StochasticRaytracingApproximation::BI_LINEAR, "bilinear", 2},
    {StochasticRaytracingApproximation::QUADRATIC, "quadratic", 2},
    {StochasticRaytracingApproximation::CUBIC, "cubic", 2},
    {0, nullptr, 0}
};

EnumDesc CommandLine::clusteringValues[] = {
    {HierarchyClusteringMode::NO_CLUSTERING, "none", 2},
    {HierarchyClusteringMode::ISOTROPIC_CLUSTERING, "isotropic", 2},
    {HierarchyClusteringMode::ORIENTED_CLUSTERING, "oriented",  2},
    {0, nullptr, 0}
};

EnumDesc CommandLine::sequenceValues[] = {
    {Sampler4DSequence::RANDOM, "PseudoRandom", 2},
    {Sampler4DSequence::HALTON,"Halton", 2},
    {Sampler4DSequence::NIEDERREITER, "Niederreiter", 2}, // TODO: Not able to select all available sequences...
    {0, nullptr, 0}
};

EnumDesc CommandLine::estimatorTypeValues[] = {
    {RandomWalkEstimatorType::RW_SHOOTING, "Shooting", 2},
    {RandomWalkEstimatorType::RW_GATHERING, "Gathering", 2},
    {0, nullptr, 0}
};

EnumDesc CommandLine::estimatorKindValues[] = {
    {RandomWalkEstimatorKind::RW_COLLISION, "Collision", 2},
    {RandomWalkEstimatorKind::RW_ABSORPTION, "Absorption", 2},
    {RandomWalkEstimatorKind::RW_SURVIVAL, "Survival", 2},
    {RandomWalkEstimatorKind::RW_LAST_BUT_NTH, "Last-but-N", 2},
    {RandomWalkEstimatorKind::RW_N_LAST, "Last-N", 2},
    {0, nullptr, 0}
};

EnumDesc CommandLine::showWhatValues[] = {
    {WhatToShow::SHOW_TOTAL_RADIANCE, "total-radiance", 2},
    {WhatToShow::SHOW_INDIRECT_RADIANCE, "indirect-radiance", 2},
    {WhatToShow::SHOW_IMPORTANCE, "importance", 2},
    {0, nullptr, 0}
};

void
CommandLine::stochasticRelaxationRadiosityParseOptions(
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

void
CommandLine::randomWalkRadiosityParseOptions(
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

EnumDesc CommandLine::rayMatterPixelFilterValues[] = {
    {RayMatterFilterType::BOX_FILTER, "box", 2},
    {RayMatterFilterType::TENT_FILTER, "tent", 2},
    {RayMatterFilterType::GAUSS_FILTER, "gaussian 1/sqrt2", 2},
    {RayMatterFilterType::GAUSS2_FILTER, "gaussian 1/2", 2},
    {0, nullptr, 0}
};

void
CommandLine::rayMattingParseOptions(
        int *argc,
        char **argv,
        RayMatterState &rayMatterState)
{
    EnumBinding<RayMatterFilterType> pixelFilterBinding = {&rayMatterState.filter, rayMatterPixelFilterValues};
    TypedOption<int> rmSamplesOpt = {"-rm-samples-per-pixel", &rayMatterState.samplesPerPixel, 1, nullptr, nullptr};
    TypedOption<EnumBinding<RayMatterFilterType>> rmPixelFilterOpt = {"-rm-pixel-filter", &pixelFilterBinding, 1, nullptr, parseEnumBinding<RayMatterFilterType>};
    OptionBase rayMatterOptions[] = {
        REGISTER_OPTION(int, rmSamplesOpt, 6),
        REGISTER_OPTION(EnumBinding<RayMatterFilterType>, rmPixelFilterOpt, 7)
    };

    OptionGroup rayMatterGroups[] = {
        OptionGroup("rayMatter", rayMatterOptions, 2)
    };
    OptionParser<OptionBase>::parse(argc, argv, rayMatterGroups, 1);
}

/*** Enum Option types ***/

EnumDesc CommandLine::rayTracingRadianceModeValues[] = {
    {RayTracingRadMode::STORED_NONE, "none", 2},
    {RayTracingRadMode::STORED_DIRECT, "direct", 2},
    {RayTracingRadMode::STORED_INDIRECT, "indirect", 2},
    {RayTracingRadMode::STORED_PHOTON_MAP, "photonmap", 2},
    {0, nullptr, 0}
};


EnumDesc CommandLine::rayTracingLightModeValues[] = {
    {RayTracingLightMode::POWER_LIGHTS, "power", 2},
    {RayTracingLightMode::IMPORTANT_LIGHTS, "important", 2},
    {RayTracingLightMode::ALL_LIGHTS, "all", 2},
    {0, nullptr, 0}
};


EnumDesc CommandLine::rayTracingSamplingModeValues[] = {
    {RayTracingSamplingMode::BRDF_SAMPLING, "bsdf", 2},
    {RayTracingSamplingMode::CLASSICAL_SAMPLING, "classical", 2},
    {0, nullptr, 0}
};

void
CommandLine::stochasticRayTracerParseOptions(
        int *argc,
        char **argv,
        StochasticRayTracingState &stochasticRayTracingState)
{
    EnumBinding<RayTracingRadMode> radModeBinding = {&stochasticRayTracingState.radMode, rayTracingRadianceModeValues};
    EnumBinding<RayTracingLightMode> lightModeBinding = {&stochasticRayTracingState.lightMode, rayTracingLightModeValues};
    EnumBinding<RayTracingSamplingMode> samplingModeBinding = {&stochasticRayTracingState.reflectionSampling, rayTracingSamplingModeValues};
    TypedOption<int> rtsSamplesPerPixelOpt = {"-rts-samples-per-pixel", &stochasticRayTracingState.samplesPerPixel, 1, nullptr, nullptr};
    TypedOption<int> rtsNoProgressiveOpt = {"-rts-no-progressive", &stochasticRayTracingState.progressiveTracing, 0, setIntFalse, nullptr};
    TypedOption<EnumBinding<RayTracingRadMode>> rtsRadModeOpt = {"-rts-rad-mode", &radModeBinding, 1, nullptr, parseEnumBinding<RayTracingRadMode>};
    TypedOption<int> rtsNoLightSamplingOpt = {"-rts-no-lightsampling", &stochasticRayTracingState.nextEvent, 0, setIntFalse, nullptr};
    TypedOption<EnumBinding<RayTracingLightMode>> rtsLightModeOpt = {"-rts-l-mode", &lightModeBinding, 1, nullptr, parseEnumBinding<RayTracingLightMode>};
    TypedOption<int> rtsLightSamplesOpt = {"-rts-l-samples", &stochasticRayTracingState.nextEventSamples, 1, nullptr, nullptr};
    TypedOption<int> rtsScatterSamplesOpt = {"-rts-scatter-samples", &stochasticRayTracingState.scatterSamples, 1, nullptr, nullptr};
    TypedOption<int> rtsDoFdgOpt = {"-rts-do-fdg", &stochasticRayTracingState.differentFirstDG, 0, setIntTrue, nullptr};
    TypedOption<int> rtsFdgSamplesOpt = {"-rts-fdg-samples", &stochasticRayTracingState.firstDGSamples, 1, nullptr, nullptr};
    TypedOption<int> rtsSeparateSpecularOpt = {"-rts-separate-specular", &stochasticRayTracingState.separateSpecular, 0, setIntTrue, nullptr};
    TypedOption<EnumBinding<RayTracingSamplingMode>> rtsSamplingModeOpt = {"-rts-s-mode", &samplingModeBinding, 1, nullptr, parseEnumBinding<RayTracingSamplingMode>};
    TypedOption<int> rtsMinPathLengthOpt = {"-rts-min-path-length", &stochasticRayTracingState.minPathDepth, 1, nullptr, nullptr};
    TypedOption<int> rtsMaxPathLengthOpt = {"-rts-max-path-length", &stochasticRayTracingState.maxPathDepth, 1, nullptr, nullptr};
    TypedOption<int> rtsNoDirectBackgroundOpt = {"-rts-NOdirect-background-rad", &stochasticRayTracingState.backgroundDirect, 0, setIntFalse, nullptr};
    TypedOption<int> rtsNoIndirectBackgroundOpt = {"-rts-NOindirect-background-rad", &stochasticRayTracingState.backgroundIndirect, 0, setIntFalse, nullptr};
    OptionBase stochasticRatTracerOptions[] = {
        REGISTER_OPTION(int, rtsSamplesPerPixelOpt, 7),
        REGISTER_OPTION(int, rtsNoProgressiveOpt, 9),
        REGISTER_OPTION(EnumBinding<RayTracingRadMode>, rtsRadModeOpt, 8),
        REGISTER_OPTION(int, rtsNoLightSamplingOpt, 9),
        REGISTER_OPTION(EnumBinding<RayTracingLightMode>, rtsLightModeOpt, 8),
        REGISTER_OPTION(int, rtsLightSamplesOpt, 8),
        REGISTER_OPTION(int, rtsScatterSamplesOpt, 7),
        REGISTER_OPTION(int, rtsDoFdgOpt, 0),
        REGISTER_OPTION(int, rtsFdgSamplesOpt, 8),
        REGISTER_OPTION(int, rtsSeparateSpecularOpt, 8),
        REGISTER_OPTION(EnumBinding<RayTracingSamplingMode>, rtsSamplingModeOpt, 9),
        REGISTER_OPTION(int, rtsMinPathLengthOpt, 8),
        REGISTER_OPTION(int, rtsMaxPathLengthOpt, 8),
        REGISTER_OPTION(int, rtsNoDirectBackgroundOpt, 8),
        REGISTER_OPTION(int, rtsNoIndirectBackgroundOpt, 8)
    };

    OptionGroup stochasticRaytracerGroups[] = {
        OptionGroup("stochasticRaytracer", stochasticRatTracerOptions, 15)
    };
    OptionParser<OptionBase>::parse(argc, argv, stochasticRaytracerGroups, 1);
}

int CommandLine::regExpStringLength = BidirectionalPathRaytracerConfig::MAX_REGEXP_SIZE;

void
CommandLine::biDirectionalPathParseOptions(
        int *argc,
        char **argv,
        BidirectionalPathTracingState &bidirectionalPathState)
{
    FixedStringBinding leBinding = {bidirectionalPathState.baseConfig.leRegExp, regExpStringLength};
    FixedStringBinding ldBinding = {bidirectionalPathState.baseConfig.ldRegExp, regExpStringLength};
    FixedStringBinding liBinding = {bidirectionalPathState.baseConfig.liRegExp, regExpStringLength};
    TypedOption<int> bidirSamplesPerPixelOpt = {"-bidir-samples-per-pixel", &bidirectionalPathState.baseConfig.samplesPerPixel, 1, nullptr, nullptr};
    TypedOption<int> bidirNoProgressiveOpt = {"-bidir-no-progressive", &bidirectionalPathState.baseConfig.progressiveTracing, 0, setIntFalse, nullptr};
    TypedOption<int> bidirMaxEyePathLengthOpt = {"-bidir-max-eye-path-length", &bidirectionalPathState.baseConfig.maximumEyePathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirMaxLightPathLengthOpt = {"-bidir-max-light-path-length", &bidirectionalPathState.baseConfig.maximumLightPathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirMaxPathLengthOpt = {"-bidir-max-path-length", &bidirectionalPathState.baseConfig.maximumPathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirMinPathLengthOpt = {"-bidir-min-path-length", &bidirectionalPathState.baseConfig.minimumPathDepth, 1, nullptr, nullptr};
    TypedOption<int> bidirNoLightImportanceOpt = {"-bidir-no-light-importance", &bidirectionalPathState.baseConfig.sampleImportantLights, 0, setIntFalse, nullptr};
    TypedOption<int> bidirUseRegexpOpt = {"-bidir-use-regexp", &bidirectionalPathState.baseConfig.useSpars, 0, setIntTrue, nullptr};
    TypedOption<int> bidirUseEmittedOpt = {"-bidir-use-emitted", &bidirectionalPathState.baseConfig.doLe, 1, nullptr, parseBoolInt};
    TypedOption<FixedStringBinding> bidirRexpEmittedOpt = {"-bidir-rexp-emitted", &leBinding, 1, nullptr, parseFixedStringBinding};
    TypedOption<int> bidirRegDirectOpt = {"-bidir-reg-direct", &bidirectionalPathState.baseConfig.doLD, 1, nullptr, parseBoolInt};
    TypedOption<FixedStringBinding> bidirRexpDirectOpt = {"-bidir-rexp-direct", &ldBinding, 1, nullptr, parseFixedStringBinding};
    TypedOption<int> bidirRegIndirectOpt = {"-bidir-reg-indirect", &bidirectionalPathState.baseConfig.doLI, 1, nullptr, parseBoolInt};
    TypedOption<FixedStringBinding> bidirRexpIndirectOpt = {"-bidir-rexp-indirect", &liBinding, 1, nullptr, parseFixedStringBinding};
    OptionBase bidirectionalOptions[] = {
        REGISTER_OPTION(int, bidirSamplesPerPixelOpt, 8),
        REGISTER_OPTION(int, bidirNoProgressiveOpt, 11),
        REGISTER_OPTION(int, bidirMaxEyePathLengthOpt, 12),
        REGISTER_OPTION(int, bidirMaxLightPathLengthOpt, 12),
        REGISTER_OPTION(int, bidirMaxPathLengthOpt, 12),
        REGISTER_OPTION(int, bidirMinPathLengthOpt, 12),
        REGISTER_OPTION(int, bidirNoLightImportanceOpt, 11),
        REGISTER_OPTION(int, bidirUseRegexpOpt, 12),
        REGISTER_OPTION(int, bidirUseEmittedOpt, 12),
        REGISTER_OPTION(FixedStringBinding, bidirRexpEmittedOpt, 13),
        REGISTER_OPTION(int, bidirRegDirectOpt, 12),
        REGISTER_OPTION(FixedStringBinding, bidirRexpDirectOpt, 13),
        REGISTER_OPTION(int, bidirRegIndirectOpt, 12),
        REGISTER_OPTION(FixedStringBinding, bidirRexpIndirectOpt, 13)
    };

    OptionGroup bidirectionalGroups[] = {
        OptionGroup("bidirectional", bidirectionalOptions, 14)
    };
    OptionParser<OptionBase>::parse(argc, argv, bidirectionalGroups, 1);
}

void
CommandLine::photonMapParseOptions(
        int *argc,
        char **argv,
        PhotonMapState &photonMapState)
{
    TypedOption<int> pmapDoGlobalOpt = {"-pmap-do-global", &photonMapState.doGlobalMap, 1, nullptr, parseBoolInt};
    TypedOption<long> pmapGlobalPathsOpt = {"-pmap-global-paths", &photonMapState.gPathsPerIteration, 1, nullptr, nullptr};
    TypedOption<int> pmapGPreirradianceOpt = {"-pmap-g-preirradiance", &photonMapState.precomputeGIrradiance, 1, nullptr, parseBoolInt};
    TypedOption<int> pmapDoCausticOpt = {"-pmap-do-caustic", &photonMapState.doCausticMap, 1, nullptr, parseBoolInt};
    TypedOption<long> pmapCausticPathsOpt = {"-pmap-caustic-paths", &photonMapState.cPathsPerIteration, 1, nullptr, nullptr};
    TypedOption<int> pmapRenderHitsOpt = {"-pmap-render-hits", &photonMapState.renderImage, 0, setIntTrue, nullptr};
    TypedOption<int> pmapReconGPhotonsOpt = {"-pmap-recon-gphotons", &photonMapState.reconGPhotons, 1, nullptr, nullptr};
    TypedOption<int> pmapReconIPhotonsOpt = {"-pmap-recon-iphotons", &photonMapState.reconCPhotons, 1, nullptr, nullptr};
    TypedOption<int> pmapReconPhotonsOpt = {"-pmap-recon-photons", &photonMapState.reconIPhotons, 1, nullptr, nullptr};
    TypedOption<int> pmapBalancingOpt = {"-pmap-balancing", &photonMapState.balanceKDTree, 1, nullptr, parseBoolInt};
    OptionBase photonMapOptions[] = {
        REGISTER_OPTION(int, pmapDoGlobalOpt, 9),
        REGISTER_OPTION(long, pmapGlobalPathsOpt, 9),
        REGISTER_OPTION(int, pmapGPreirradianceOpt, 11),
        REGISTER_OPTION(int, pmapDoCausticOpt, 9),
        REGISTER_OPTION(long, pmapCausticPathsOpt, 9),
        REGISTER_OPTION(int, pmapRenderHitsOpt, 9),
        REGISTER_OPTION(int, pmapReconGPhotonsOpt, 9),
        REGISTER_OPTION(int, pmapReconIPhotonsOpt, 9),
        REGISTER_OPTION(int, pmapReconPhotonsOpt, 9),
        REGISTER_OPTION(int, pmapBalancingOpt, 9)
    };
    OptionGroup photonMapGroups[] = {
        OptionGroup("photonMap", photonMapOptions, 10)
    };
    OptionParser<OptionBase>::parse(argc, argv, photonMapGroups, 1);
}

#endif
