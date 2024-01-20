#include <cstring>

#include "common/error.h"
#include "shared/options.h"
#include "shared/defaults.h"
#include "IMAGE/tonemap/dummy.h"
#include "IMAGE/tonemap/lightness.h"
#include "IMAGE/tonemap/trwf.h"
#include "IMAGE/tonemap/tonemapping.h"

#define DEFAULT_GAMMA 1.7

TONEMAP *GLOBAL_toneMap_availableToneMaps[] = {
    &GLOBAL_toneMap_lightness,
    &GLOBAL_toneMap_tumblinRushmeier,
    &GLOBAL_toneMap_ward,
    &GLOBAL_toneMap_revisedTumblinRushmeier,
    &GLOBAL_toneMap_ferwerda,
    (TONEMAP *) nullptr
};

// Tone mapping context
TONEMAPPINGCONTEXT GLOBAL_toneMap_options;

// Composes explanation for -tonemapping command line option
#define STRING_SIZE 1000
static char globalToneMappingMethodsString[STRING_SIZE];
static float globalRxy[2];
static float globalGxy[2];
static float globalBxy[2];
static float globalWxy[2];

static void
makeToneMappingMethodsString() {
    char *str = globalToneMappingMethodsString;
    int n;
    int first = true;
    snprintf(str, 1000, "-tonemapping <method>: Set tone mapping method\n%n", &n);
    str += n;
    snprintf(str, 1000, "\tmethods: %n", &n);
    str += n;

    ForAllAvailableToneMaps(method)
                {
                    if ( !first ) {
                        snprintf(str, STRING_SIZE, "\t         %n", &n);
                        str += n;
                    }
                    first = false;
                    snprintf(str, STRING_SIZE, "%-20.20s %s%s\n%n",
                            method->shortName, method->name,
                            GLOBAL_toneMap_options.ToneMap == method ? " (default)" : "", &n);
                    str += n;
                }
    EndForAll;
    *(str - 1) = '\0';    /* discard last newline character */
}

static void
toneMappingMethodOption(void *value) {
    char *name = *(char **) value;

    ForAllAvailableToneMaps(method)
                {
                    if ( strncasecmp(name, method->shortName, method->abbrev) == 0 ) {
                        setToneMap(method);
                        return;
                    }
                }
    EndForAll;

    logError(nullptr, "Invalid tone mapping method name '%s'", name);
}

static void
brightnessAdjustOption(void *val) {
    GLOBAL_toneMap_options.pow_bright_adjust = (float)std::pow(2.0, GLOBAL_toneMap_options.brightness_adjust);
}

static void
chromaOption(void *value) {
    float *chroma = (float *) value;
    if ( chroma == globalRxy ) {
        GLOBAL_toneMap_options.xr = chroma[0];
        GLOBAL_toneMap_options.yr = chroma[1];
    } else if ( chroma == globalGxy ) {
        GLOBAL_toneMap_options.xg = chroma[0];
        GLOBAL_toneMap_options.yg = chroma[1];
    } else if ( chroma == globalBxy ) {
        GLOBAL_toneMap_options.xb = chroma[0];
        GLOBAL_toneMap_options.yb = chroma[1];
    } else if ( chroma == globalWxy ) {
        GLOBAL_toneMap_options.xw = chroma[0];
        GLOBAL_toneMap_options.yw = chroma[1];
    } else {
        logFatal(-1, "chromaOption", "invalid value pointer");
    }

    computeColorConversionTransforms(GLOBAL_toneMap_options.xr, GLOBAL_toneMap_options.yr,
                                     GLOBAL_toneMap_options.xg, GLOBAL_toneMap_options.yg,
                                     GLOBAL_toneMap_options.xb, GLOBAL_toneMap_options.yb,
                                     GLOBAL_toneMap_options.xw, GLOBAL_toneMap_options.yw);
}

static void
toneMappingCommandLineOptionDescAdaptMethodOption(void *value) {
    char *name = *(char **) value;

    if ( strncasecmp(name, "average", 2) == 0 ) {
        GLOBAL_toneMap_options.statadapt = TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        GLOBAL_toneMap_options.statadapt = TMA_MEDIAN;
    } else {
        logError(nullptr,
                 "Invalid adaptation estimate method '%s'",
                 name);
    }
}

static void
gammaOption(void *value) {
    float gam = *(float *) value;
    RGBSET(GLOBAL_toneMap_options.gamma, gam, gam, gam);
}

static CommandLineOptionDescription globalToneMappingOptions[] = {
{"-tonemapping",       4, Tstring,
    nullptr, toneMappingMethodOption,
    globalToneMappingMethodsString},
{"-brightness-adjust", 4, Tfloat,
    &GLOBAL_toneMap_options.brightness_adjust, brightnessAdjustOption,
    "-brightness-adjust <float> : brightness adjustment factor"},
{"-adapt",             5, Tstring,
    nullptr, toneMappingCommandLineOptionDescAdaptMethodOption,
    "-adapt <method>  \t: adaptation estimation method\n"
    "\tmethods: \"average\", \"median\""},
{"-lwa",               3, Tfloat,
    &GLOBAL_toneMap_options.lwa,   DEFAULT_ACTION,
    "-lwa <float>\t\t: real world adaptation luminance"},
{"-ldmax",             5, Tfloat,
    &GLOBAL_toneMap_options.ldm,   DEFAULT_ACTION,
    "-ldmax <float>\t\t: maximum diaply luminance"},
{"-cmax",              4, Tfloat,
    &GLOBAL_toneMap_options.cmax,  DEFAULT_ACTION,
    "-cmax <float>\t\t: maximum displayable contrast"},
{"-gamma",             4, Tfloat,
    nullptr, gammaOption,
    "-gamma <float>       \t: gamma correction factor (same for red, green. blue)"},
{"-rgbgamma",          4, TRGB,
    &GLOBAL_toneMap_options.gamma, DEFAULT_ACTION,
    "-rgbgamma <r> <g> <b>\t: gamma correction factor (separate for red, green, blue)"},
{"-red",               4, Txy,
    globalRxy, chromaOption,
    "-red <xy>            \t: CIE xy chromaticity of monitor red"},
{"-green",             4, Txy,
    globalGxy, chromaOption,
    "-green <xy>          \t: CIE xy chromaticity of monitor green"},
{"-blue",              4, Txy,
    globalBxy, chromaOption,
    "-blue <xy>           \t: CIE xy chromaticity of monitor blue"},
{"-white",             4, Txy,
    globalWxy, chromaOption,
    "-white <xy>          \t: CIE xy chromaticity of monitor white"},
{nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

void
toneMapDefaults() {
    ForAllAvailableToneMaps(map)
                {
                    map->Defaults();
                }
    EndForAll;

    GLOBAL_toneMap_options.brightness_adjust = 0.;
    GLOBAL_toneMap_options.pow_bright_adjust = pow(2., GLOBAL_toneMap_options.brightness_adjust);

    GLOBAL_toneMap_options.statadapt = TMA_MEDIAN;
    GLOBAL_toneMap_options.lwa = DEFAULT_TM_LWA;
    GLOBAL_toneMap_options.ldm = DEFAULT_TM_LDMAX;
    GLOBAL_toneMap_options.cmax = DEFAULT_TM_CMAX;

    globalRxy[0] = GLOBAL_toneMap_options.xr = 0.640;
    globalRxy[1] = GLOBAL_toneMap_options.yr = 0.330;
    globalGxy[0] = GLOBAL_toneMap_options.xg = 0.290;
    globalGxy[1] = GLOBAL_toneMap_options.yg = 0.600;
    globalBxy[0] = GLOBAL_toneMap_options.xb = 0.150;
    globalBxy[1] = GLOBAL_toneMap_options.yb = 0.060;
    globalWxy[0] = GLOBAL_toneMap_options.xw = 0.333333333333;
    globalWxy[1] = GLOBAL_toneMap_options.yw = 0.333333333333;
    computeColorConversionTransforms(GLOBAL_toneMap_options.xr, GLOBAL_toneMap_options.yr,
                                     GLOBAL_toneMap_options.xg, GLOBAL_toneMap_options.yg,
                                     GLOBAL_toneMap_options.xb, GLOBAL_toneMap_options.yb,
                                     GLOBAL_toneMap_options.xw, GLOBAL_toneMap_options.yw);

    RGBSET(GLOBAL_toneMap_options.gamma, DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA);
    recomputeGammaTables(GLOBAL_toneMap_options.gamma);
    GLOBAL_toneMap_options.ToneMap = &GLOBAL_toneMap_lightness;
    GLOBAL_toneMap_options.ToneMap->Init();

    makeToneMappingMethodsString();
}

void
parseToneMapOptions(int *argc, char **argv) {
    parseOptions(globalToneMappingOptions, argc, argv);
    recomputeGammaTables(GLOBAL_toneMap_options.gamma);

    ForAllAvailableToneMaps(map)
                {
                    if ( map->ParseOptions ) {
                        map->ParseOptions(argc, argv);
                    }
                }
    EndForAll;
}

/**
Makes map the current tone mapping operator + initialises
*/
void
setToneMap(TONEMAP *map) {
    GLOBAL_toneMap_options.ToneMap->Terminate();
    GLOBAL_toneMap_options.ToneMap = map ? map : &GLOBAL_toneMap_dummy;
    GLOBAL_toneMap_options.ToneMap->Init();
}

/**
Initialises tone mapping, e.g. for a new scene
*/
void
initToneMapping() {
    initSceneAdaptation();
    setToneMap(GLOBAL_toneMap_options.ToneMap);
}

void
recomputeGammaTable(int index, double gamma) {
    int i;
    if ( gamma <= EPSILON ) {
        gamma = 1.;
    }
    for ( i = 0; i <= (1 << GAMMATAB_BITS); i++ ) {
        GLOBAL_toneMap_options.gammatab[index][i] = (float)std::pow((double) i / (double) (1 << GAMMATAB_BITS), 1. / gamma);
    }
}

/**
Recomputes gamma tables for the given gamma values for red, green and blue
*/
void
recomputeGammaTables(RGB gamma) {
    recomputeGammaTable(0, gamma.r);
    recomputeGammaTable(1, gamma.g);
    recomputeGammaTable(2, gamma.b);
}

/**
Rescale real world radiance using appropiately set up tone mapping algorithm
*/
COLOR *
rescaleRadiance(COLOR in, COLOR *out) {
    colorScale(GLOBAL_toneMap_options.pow_bright_adjust, in, in);
    *out = toneMapScaleForDisplay(in);
    return out;
}

RGB *
radianceToRgb(COLOR color, RGB *rgb) {
    rescaleRadiance(color, &color);
    convertColorToRGB(color, rgb);
    rgb->clip();
    return rgb;
}
