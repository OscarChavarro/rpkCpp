#include <cstring>

#include "vsdk/toolkit/common/commandLineOptions/OptionParser.h"
#include "vsdk/toolkit/common/commandLineOptions/TypedOption.h"
#include "options/OptionsGroupStochasticRaytracing.h"

template<typename T>
bool OptionsGroupStochasticRaytracing::parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
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

void
OptionsGroupStochasticRaytracing::setIntTrue(int &value) {
    value = 1;
}

void
OptionsGroupStochasticRaytracing::setIntFalse(int &value) {
    value = 0;
}

EnumDesc OptionsGroupStochasticRaytracing::rayTracingRadianceModeValues[] = {
    {RayTracingRadMode::STORED_NONE, "none", 2},
    {RayTracingRadMode::STORED_DIRECT, "direct", 2},
    {RayTracingRadMode::STORED_INDIRECT, "indirect", 2},
    {RayTracingRadMode::STORED_PHOTON_MAP, "photonmap", 2},
    {0, nullptr, 0}
};

EnumDesc OptionsGroupStochasticRaytracing::rayTracingLightModeValues[] = {
    {RayTracingLightMode::POWER_LIGHTS, "power", 2},
    {RayTracingLightMode::IMPORTANT_LIGHTS, "important", 2},
    {RayTracingLightMode::ALL_LIGHTS, "all", 2},
    {0, nullptr, 0}
};

EnumDesc OptionsGroupStochasticRaytracing::rayTracingSamplingModeValues[] = {
    {RayTracingSamplingMode::BRDF_SAMPLING, "bsdf", 2},
    {RayTracingSamplingMode::CLASSICAL_SAMPLING, "classical", 2},
    {0, nullptr, 0}
};

void
OptionsGroupStochasticRaytracing::parse(
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
