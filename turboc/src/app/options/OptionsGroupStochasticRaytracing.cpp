#include <cstrings>

#include "common/commandLineOptions/OptionParser.h"
#include "common/commandLineOptions/TypedOption.h"
#include "app/options/OptionsGroupStochasticRaytracing.h"

template<typename T>
bool OptsGrpStochRaytr::parseEnumBinding(int argc, char **argv, EnumBinding<T> &binding) {
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

void
OptsGrpStochRaytr::setIntTrue(int &value) {
    value = 1;
}

void
OptsGrpStochRaytr::setIntFalse(int &value) {
    value = 0;
}

EnumDesc OptsGrpStochRaytr::rayTracingRadianceModeValues[] = {
    {STORED_NONE, "none", 2},
    {STORED_DIRECT, "direct", 2},
    {STORED_INDIRECT, "indirect", 2},
    {STORED_PHOTON_MAP, "photonmap", 2},
    {0, NULL, 0}
};

EnumDesc OptsGrpStochRaytr::rayTracingLightModeValues[] = {
    {POWER_LIGHTS, "power", 2},
    {IMPORTANT_LIGHTS, "important", 2},
    {ALL_LIGHTS, "all", 2},
    {0, NULL, 0}
};

EnumDesc OptsGrpStochRaytr::rayTracingSamplingModeValues[] = {
    {BRDF_SAMPLING, "bsdf", 2},
    {CLASSICAL_SAMPLING, "classical", 2},
    {0, NULL, 0}
};

void
OptsGrpStochRaytr::parse(
        int *argc,
        char **argv,
        StochasticRayTracingState &stochasticRayTracingState)
{
    EnumBinding<RayTracingRadMode> radModeBinding = {&stochasticRayTracingState.radMode, rayTracingRadianceModeValues};
    EnumBinding<RayTracingLightMode> lightModeBinding = {&stochasticRayTracingState.lightMode, rayTracingLightModeValues};
    EnumBinding<RayTracingSamplingMode> samplingModeBinding = {&stochasticRayTracingState.reflectionSampling, rayTracingSamplingModeValues};
    TypedOption<int> rtsSamplesPerPixelOpt("-rts-samples-per-pixel", &stochasticRayTracingState.samplesPerPixel, 1, NULL, NULL);
    TypedOption<int> rtsNoProgressiveOpt("-rts-no-progressive", &stochasticRayTracingState.progressiveTracing, 0, setIntFalse, NULL);
    TypedOption<EnumBinding<RayTracingRadMode> > rtsRadModeOpt("-rts-rad-mode", &radModeBinding, 1, NULL, parseEnumBinding<RayTracingRadMode>);
    TypedOption<int> rtsNoLightSamplingOpt("-rts-no-lightsampling", &stochasticRayTracingState.nextEvent, 0, setIntFalse, NULL);
    TypedOption<EnumBinding<RayTracingLightMode> > rtsLightModeOpt("-rts-l-mode", &lightModeBinding, 1, NULL, parseEnumBinding<RayTracingLightMode>);
    TypedOption<int> rtsLightSamplesOpt("-rts-l-samples", &stochasticRayTracingState.nextEventSamples, 1, NULL, NULL);
    TypedOption<int> rtsScatterSamplesOpt("-rts-scatter-samples", &stochasticRayTracingState.scatterSamples, 1, NULL, NULL);
    TypedOption<int> rtsDoFdgOpt("-rts-do-fdg", &stochasticRayTracingState.differentFirstDG, 0, setIntTrue, NULL);
    TypedOption<int> rtsFdgSamplesOpt("-rts-fdg-samples", &stochasticRayTracingState.firstDGSamples, 1, NULL, NULL);
    TypedOption<int> rtsSeparateSpecularOpt("-rts-separate-specular", &stochasticRayTracingState.separateSpecular, 0, setIntTrue, NULL);
    TypedOption<EnumBinding<RayTracingSamplingMode> > rtsSamplingModeOpt("-rts-s-mode", &samplingModeBinding, 1, NULL, parseEnumBinding<RayTracingSamplingMode>);
    TypedOption<int> rtsMinPathLengthOpt("-rts-min-path-length", &stochasticRayTracingState.minPathDepth, 1, NULL, NULL);
    TypedOption<int> rtsMaxPathLengthOpt("-rts-max-path-length", &stochasticRayTracingState.maxPathDepth, 1, NULL, NULL);
    TypedOption<int> rtsNoDirectBackgroundOpt("-rts-NOdirect-background-rad", &stochasticRayTracingState.backgroundDirect, 0, setIntFalse, NULL);
    TypedOption<int> rtsNoIndirectBackgroundOpt("-rts-NOindirect-background-rad", &stochasticRayTracingState.backgroundIndirect, 0, setIntFalse, NULL);
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
