/**
Options defaults and parsing for stochastic raytracing
*/

#include "shared/options.h"
#include "StochasticRaytracerOptions.h"

void
stochasticRayTracerDefaults() {
    // Normal
    GLOBAL_raytracing_state.samplesPerPixel = 1;
    GLOBAL_raytracing_state.progressiveTracing = true;

    GLOBAL_raytracing_state.doFrameCoherent = false;
    GLOBAL_raytracing_state.doCorrelatedSampling = false;
    GLOBAL_raytracing_state.baseSeed = 0xFE062134;

    GLOBAL_raytracing_state.radMode = STORED_NONE;

    GLOBAL_raytracing_state.nextEvent = true;
    GLOBAL_raytracing_state.nextEventSamples = 1;
    GLOBAL_raytracing_state.lightMode = ALL_LIGHTS;

    GLOBAL_raytracing_state.backgroundDirect = false;
    GLOBAL_raytracing_state.backgroundIndirect = true;
    GLOBAL_raytracing_state.backgroundSampling = false;

    GLOBAL_raytracing_state.scatterSamples = 1;
    GLOBAL_raytracing_state.differentFirstDG = false;
    GLOBAL_raytracing_state.firstDGSamples = 36;
    GLOBAL_raytracing_state.separateSpecular = false;

    GLOBAL_raytracing_state.reflectionSampling = BRDFSAMPLING;

    GLOBAL_raytracing_state.minPathDepth = 5;
    GLOBAL_raytracing_state.maxPathDepth = 7;

    // Common
    GLOBAL_raytracing_state.lastscreen = (ScreenBuffer *) 0;
}


/*** Enum Option types ***/

static ENUMDESC radModeVals[] = {
        {STORED_NONE,      "none",      2},
        {STORED_DIRECT,    "direct",    2},
        {STORED_INDIRECT,  "indirect",  2},
        {STORED_PHOTONMAP, "photonmap", 2},
        {0, nullptr,                       0}
};
MakeEnumOptTypeStruct(radModeTypeStruct, radModeVals);
#define TradMode (&radModeTypeStruct)

static ENUMDESC lightModeVals[] = {
        {POWER_LIGHTS,     "power",     2},
        {IMPORTANT_LIGHTS, "important", 2},
        {ALL_LIGHTS,       "all",       2},
        {0, nullptr,                       0}
};
MakeEnumOptTypeStruct(lightModeTypeStruct, lightModeVals);
#define TlightMode (&lightModeTypeStruct)

static ENUMDESC samplingModeVals[] = {
        {BRDFSAMPLING,      "bsdf",      2},
        {CLASSICALSAMPLING, "classical", 2},
        {0, nullptr,                        0}
};
MakeEnumOptTypeStruct(samplingModeTypeStruct, samplingModeVals);
#define TsamplingMode (&samplingModeTypeStruct)

static CMDLINEOPTDESC globalStochasticRatTracerOptions[] = {
        {"-rts-samples-per-pixel", 7, Tint, &GLOBAL_raytracing_state.samplesPerPixel, DEFAULT_ACTION,
         "-rts-samples-per-pixel <number>\t: eye-rays per pixel"},
        {"-rts-no-progressive", 9, Tsetfalse, &GLOBAL_raytracing_state.progressiveTracing, DEFAULT_ACTION,
         "-rts-no-progressive\t: don't do progressive image refinement"},
        {"-rts-rad-mode", 8, TradMode, &GLOBAL_raytracing_state.radMode, DEFAULT_ACTION,
         "-rts-rad-mode <type>\t: Stored radiance usage - \"none\", \"direct\", \"indirect\", \"photonmap\""},
        {"-rts-no-lightsampling", 9, Tsetfalse, &GLOBAL_raytracing_state.nextEvent, DEFAULT_ACTION,
         "-rts-no-lightsampling\t: don't do explicit light sampling"},
        {"-rts-l-mode", 8, TlightMode, &GLOBAL_raytracing_state.lightMode, DEFAULT_ACTION,
         "-rts-l-mode <type>\t: Light sampling mode - \"power\", \"important\", \"all\""},
        {"-rts-l-samples", 8, Tint, &GLOBAL_raytracing_state.nextEventSamples, DEFAULT_ACTION,
         "-rts-l-samples <number>\t: explicit light source samples at each hit"},
        {"-rts-scatter-samples", 7, Tint, &GLOBAL_raytracing_state.scatterSamples, DEFAULT_ACTION,
         "-rts-scatter-samples <number>\t: scattered rays at each bounce"},
        {"-rts-do-fdg", 0, Tsettrue, &GLOBAL_raytracing_state.differentFirstDG, DEFAULT_ACTION,
         "-rts-do-fdg\t: use different nr. of scatter samples for first diffuse/glossy bounce"},
        {"-rts-fdg-samples", 8, Tint, &GLOBAL_raytracing_state.firstDGSamples, DEFAULT_ACTION,
         "-rts-fdg-samples <number>\t: scattered rays at first diffuse/glossy bounce"},
        {"-rts-separate-specular", 8, Tsettrue, &GLOBAL_raytracing_state.separateSpecular, DEFAULT_ACTION,
         "-rts-separate-specular\t: always shoot separate rays for specular scattering"},
        {"-rts-s-mode", 9, TsamplingMode, &GLOBAL_raytracing_state.reflectionSampling, DEFAULT_ACTION,
         "-rts-s-mode <type>\t: Sampling mode - \"bsdf\", \"classical\""},
        {"-rts-min-path-length", 8, Tint, &GLOBAL_raytracing_state.minPathDepth, DEFAULT_ACTION,
         "-rts-min-path-length <number>\t: minimum path length before Russian roulette"},
        {"-rts-max-path-length", 8, Tint, &GLOBAL_raytracing_state.maxPathDepth, DEFAULT_ACTION,
         "-rts-max-path-length <number>\t: maximum path length (ignoring higher orders)"},
        {"-rts-NOdirect-background-rad", 8, Tsetfalse, &GLOBAL_raytracing_state.backgroundDirect, DEFAULT_ACTION,
         "-rts-NOdirect-background-rad\t: omit direct background radiance."},
        {"-rts-NOindirect-background-rad", 8, Tsetfalse, &GLOBAL_raytracing_state.backgroundIndirect, DEFAULT_ACTION,
         "-rts-NOindirect-background-rad\t: omit indirect background radiance."},
        {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
RTStochasticParseOptions(int *argc, char **argv) {
    ParseOptions(globalStochasticRatTracerOptions, argc, argv);
}
