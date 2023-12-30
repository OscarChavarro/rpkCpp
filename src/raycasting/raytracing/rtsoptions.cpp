/* rtsoptions.C: Options defaults and parsing for
   stochastic raytracing */

#include "shared/options.h"
#include "raycasting/raytracing/rtsoptions.h"

void RTStochasticDefaults() {
    /* NORMAL */

    rts.samplesPerPixel = 1;
    rts.progressiveTracing = true;

    rts.doFrameCoherent = false;
    rts.doCorrelatedSampling = false;
    rts.baseSeed = 0xFE062134;

    rts.radMode = STORED_NONE;

    rts.nextEvent = true;
    rts.nextEventSamples = 1;
    rts.lightMode = ALL_LIGHTS;

    rts.backgroundDirect = false;
    rts.backgroundIndirect = true;
    rts.backgroundSampling = false;

    rts.scatterSamples = 1;
    rts.differentFirstDG = false;
    rts.firstDGSamples = 36;
    rts.separateSpecular = false;

    rts.reflectionSampling = BRDFSAMPLING;

    rts.minPathDepth = 5;
    rts.maxPathDepth = 7;

    // Common
    rts.lastscreen = (ScreenBuffer *) 0;
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


static CMDLINEOPTDESC rtsOptions[] =
        {
                {"-rts-samples-per-pixel",         7, Tint,          &rts.samplesPerPixel,    DEFAULT_ACTION,
                        "-rts-samples-per-pixel <number>\t: eye-rays per pixel"},
                {"-rts-no-progressive",            9, Tsetfalse,     &rts.progressiveTracing, DEFAULT_ACTION,
                        "-rts-no-progressive\t: don't do progressive image refinement"},
                {"-rts-rad-mode",                  8, TradMode,      &rts.radMode,            DEFAULT_ACTION,
                        "-rts-rad-mode <type>\t: Stored radiance usage - \"none\", \"direct\", \"indirect\", \"photonmap\""},
                {"-rts-no-lightsampling",          9, Tsetfalse,     &rts.nextEvent,          DEFAULT_ACTION,
                        "-rts-no-lightsampling\t: don't do explicit light sampling"},
                {"-rts-l-mode",                    8, TlightMode,    &rts.lightMode,          DEFAULT_ACTION,
                        "-rts-l-mode <type>\t: Light sampling mode - \"power\", \"important\", \"all\""},
                {"-rts-l-samples",                 8, Tint,          &rts.nextEventSamples,   DEFAULT_ACTION,
                        "-rts-l-samples <number>\t: explicit light source samples at each hit"},
                {"-rts-scatter-samples",           7, Tint,          &rts.scatterSamples,     DEFAULT_ACTION,
                        "-rts-scatter-samples <number>\t: scattered rays at each bounce"},
                {"-rts-do-fdg",                    0, Tsettrue,      &rts.differentFirstDG,   DEFAULT_ACTION,
                        "-rts-do-fdg\t: use different nr. of scatter samples for first diffuse/glossy bounce"},
                {"-rts-fdg-samples",               8, Tint,          &rts.firstDGSamples,     DEFAULT_ACTION,
                        "-rts-fdg-samples <number>\t: scattered rays at first diffuse/glossy bounce"},
                {"-rts-separate-specular",         8, Tsettrue,      &rts.separateSpecular,   DEFAULT_ACTION,
                        "-rts-separate-specular\t: always shoot separate rays for specular scattering"},
                {"-rts-s-mode",                    9, TsamplingMode, &rts.reflectionSampling, DEFAULT_ACTION,
                        "-rts-s-mode <type>\t: Sampling mode - \"bsdf\", \"classical\""},
                {"-rts-min-path-length",           8, Tint,          &rts.minPathDepth,       DEFAULT_ACTION,
                        "-rts-min-path-length <number>\t: minimum path length before Russian roulette"},
                {"-rts-max-path-length",           8, Tint,          &rts.maxPathDepth,       DEFAULT_ACTION,
                        "-rts-max-path-length <number>\t: maximum path length (ignoring higher orders)"},
                {"-rts-NOdirect-background-rad",   8, Tsetfalse,     &rts.backgroundDirect,   DEFAULT_ACTION,
                        "-rts-NOdirect-background-rad\t: omit direct background radiance."},
                {"-rts-NOindirect-background-rad", 8, Tsetfalse,     &rts.backgroundIndirect, DEFAULT_ACTION,
                        "-rts-NOindirect-background-rad\t: omit indirect background radiance."},
                {nullptr,                             0, TYPELESS, nullptr,                         DEFAULT_ACTION,
                        nullptr}
        };

void
RTStochasticParseOptions(int *argc, char **argv) {
    ParseOptions(rtsOptions, argc, argv);
}
