#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/options.h"
#include "IMAGE/tonemap/dummy.h"
#include "IMAGE/tonemap/lightness.h"
#include "IMAGE/tonemap/trwf.h"
#include "IMAGE/tonemap/tonemapping.h"

#define DEFAULT_GAMMA 1.7

// Tone mapping defaults
#define DEFAULT_TM_LWA 10.0
#define DEFAULT_TM_LDMAX 100.0
#define DEFAULT_TM_CMAX 50.0

ToneMap *GLOBAL_toneMap_availableToneMaps[] = {
    &GLOBAL_toneMap_lightness,
    &GLOBAL_toneMap_tumblinRushmeier,
    &GLOBAL_toneMap_ward,
    &GLOBAL_toneMap_revisedTumblinRushmeier,
    &GLOBAL_toneMap_ferwerda,
    nullptr
};

// Tone mapping context
ToneMappingContext GLOBAL_toneMap_options;

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
    snprintf(str, 1000, "-tonemapping <method>: set tone mapping method\n%n", &n);
    str += n;
    snprintf(str, 1000, "\tmethods: %n", &n);
    str += n;

    for ( ToneMap **toneMap = GLOBAL_toneMap_availableToneMaps; *toneMap != nullptr; toneMap++) {
        const ToneMap *method = *toneMap;
        if ( !first ) {
            snprintf(str, STRING_SIZE, "\t         %n", &n);
            str += n;
        }
        first = false;
        snprintf(str, STRING_SIZE, "%-20.20s %s%s\n%n",
                 method->shortName, method->name,
                 GLOBAL_toneMap_options.toneMap == method ? " (default)" : "", &n);
        str += n;
    }
    *(str - 1) = '\0'; // Discard last newline character
}

static void
toneMappingMethodOption(void *value) {
    char *name = *(char **) value;

    for ( ToneMap **toneMap = GLOBAL_toneMap_availableToneMaps; *toneMap != nullptr; toneMap++) {
        ToneMap *method = *toneMap;
        if ( strncasecmp(name, method->shortName, method->abbrev) == 0 ) {
            setToneMap(method);
            return;
        }
    }

    logError(nullptr, "Invalid tone mapping method name '%s'", name);
}

static void
brightnessAdjustOption(void * /*val*/) {
    GLOBAL_toneMap_options.pow_bright_adjust = (float)java::Math::pow(2.0f, GLOBAL_toneMap_options.brightness_adjust);
}

static void
chromaOption(void *value) {
    const float *chroma = (float *)value;
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
        GLOBAL_toneMap_options.staticAdaptationMethod = TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        GLOBAL_toneMap_options.staticAdaptationMethod = TMA_MEDIAN;
    } else {
        logError(nullptr,
                 "Invalid adaptation estimate method '%s'",
                 name);
    }
}

static void
gammaOption(void *value) {
    float gam = *(float *) value;
    GLOBAL_toneMap_options.gamma.set(gam, gam, gam);
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
    &GLOBAL_toneMap_options.realWorldAdaptionLuminance, DEFAULT_ACTION,
    "-lwa <float>\t\t: real world adaptation luminance"},
{"-ldmax",             5, Tfloat,
    &GLOBAL_toneMap_options.maximumDisplayLuminance, DEFAULT_ACTION,
    "-ldmax <float>\t\t: maximum diaply luminance"},
{"-cmax",              4, Tfloat,
    &GLOBAL_toneMap_options.maximumDisplayContrast, DEFAULT_ACTION,
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

/**
rayCasterDefaults and option handling
*/
void
toneMapDefaults() {
    for ( ToneMap **toneMap = GLOBAL_toneMap_availableToneMaps; *toneMap != nullptr; toneMap++) {
        const ToneMap *map = *toneMap;
        map->Defaults();
    }

    GLOBAL_toneMap_options.brightness_adjust = 0.0;
    GLOBAL_toneMap_options.pow_bright_adjust = java::Math::pow(2.0f, GLOBAL_toneMap_options.brightness_adjust);

    GLOBAL_toneMap_options.staticAdaptationMethod = TMA_MEDIAN;
    GLOBAL_toneMap_options.realWorldAdaptionLuminance = DEFAULT_TM_LWA;
    GLOBAL_toneMap_options.maximumDisplayLuminance = DEFAULT_TM_LDMAX;
    GLOBAL_toneMap_options.maximumDisplayContrast = DEFAULT_TM_CMAX;

    globalRxy[0] = GLOBAL_toneMap_options.xr = 0.640f;
    globalRxy[1] = GLOBAL_toneMap_options.yr = 0.330f;
    globalGxy[0] = GLOBAL_toneMap_options.xg = 0.290f;
    globalGxy[1] = GLOBAL_toneMap_options.yg = 0.600f;
    globalBxy[0] = GLOBAL_toneMap_options.xb = 0.150f;
    globalBxy[1] = GLOBAL_toneMap_options.yb = 0.060f;
    globalWxy[0] = GLOBAL_toneMap_options.xw = 0.333333333333f;
    globalWxy[1] = GLOBAL_toneMap_options.yw = 0.333333333333f;
    computeColorConversionTransforms(GLOBAL_toneMap_options.xr, GLOBAL_toneMap_options.yr,
                                     GLOBAL_toneMap_options.xg, GLOBAL_toneMap_options.yg,
                                     GLOBAL_toneMap_options.xb, GLOBAL_toneMap_options.yb,
                                     GLOBAL_toneMap_options.xw, GLOBAL_toneMap_options.yw);

    GLOBAL_toneMap_options.gamma.set((float)DEFAULT_GAMMA, (float)DEFAULT_GAMMA, (float)DEFAULT_GAMMA);
    recomputeGammaTables(GLOBAL_toneMap_options.gamma);
    GLOBAL_toneMap_options.toneMap = &GLOBAL_toneMap_lightness;
    GLOBAL_toneMap_options.toneMap->Init();

    makeToneMappingMethodsString();
}

void
toneMapParseOptions(int *argc, char **argv) {
    parseGeneralOptions(globalToneMappingOptions, argc, argv);
    recomputeGammaTables(GLOBAL_toneMap_options.gamma);

    for ( ToneMap **toneMap = GLOBAL_toneMap_availableToneMaps; *toneMap != nullptr; toneMap++) {
        const ToneMap *map = *toneMap;
        if ( map->ParseOptions ) {
            map->ParseOptions(argc, argv);
        }
    }
}

/**
Makes map the current tone mapping operator + initialises
*/
void
setToneMap(ToneMap *map) {
    GLOBAL_toneMap_options.toneMap->Terminate();
    GLOBAL_toneMap_options.toneMap = map ? map : &GLOBAL_toneMap_dummy;
    GLOBAL_toneMap_options.toneMap->Init();
}

/**
Initialises tone mapping, e.g. for a new scene
*/
void
initToneMapping(java::ArrayList<Patch *> *scenePatches) {
    initSceneAdaptation(scenePatches);
    setToneMap(GLOBAL_toneMap_options.toneMap);
}

void
recomputeGammaTable(int index, double gamma) {
    if ( gamma <= EPSILON ) {
        gamma = 1.0;
    }
    for ( int i = 0; i <= (1 << GAMMA_TAB_BITS); i++ ) {
        GLOBAL_toneMap_options.gammaTab[index][i] = (float)java::Math::pow((double) i / (double) (1 << GAMMA_TAB_BITS), 1.0 / gamma);
    }
}

/**
Recomputes gamma tables for the given gamma values for red, green and blue
*/
void
recomputeGammaTables(ColorRgb gamma) {
    recomputeGammaTable(0, gamma.r);
    recomputeGammaTable(1, gamma.g);
    recomputeGammaTable(2, gamma.b);
}

/**
Rescale real world radiance using properly set up tone mapping algorithm
*/
ColorRgb *
rescaleRadiance(ColorRgb in, ColorRgb *out) {
    in.scale(GLOBAL_toneMap_options.pow_bright_adjust);
    *out = toneMapScaleForDisplay(in);
    return out;
}

ColorRgb *
radianceToRgb(ColorRgb color, ColorRgb *rgb) {
    rescaleRadiance(color, &color);
    rgb->set(color.r, color.g, color.b);
    rgb->clip();
    return rgb;
}
