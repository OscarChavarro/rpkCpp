#include <cstring>

#include "common/error.h"
#include "material/cie.h"
#include "common/mymath.h"
#include "shared/options.h"
#include "shared/defaults.h"
#include "IMAGE/tonemap/dummy.h"
#include "IMAGE/tonemap/lightness.h"
#include "trwf.h"
#include "tonemapping.h"

#define DEFAULT_GAMMA 1.7

/* available tone maps */
TONEMAP *AvailableToneMaps[] = {
        &TM_Lightness,
        &TM_TumblinRushmeier,
        &TM_Ward,
        &TM_RevisedTumblinRushmeier,
        &TM_Ferwerda,
        (TONEMAP *) nullptr        /* sentinel */
};

/* tone mapping context */
TONEMAPPINGCONTEXT tmopts;

/* composes explanation for -tonemapping command line option */
static char tonemapping_methods_string[1000];

static void make_tonemapping_methods_string() {
    char *str = tonemapping_methods_string;
    int n, first = true;
    sprintf(str, "\
-tonemapping <method>: Set tone mapping method\n%n",
            &n);
    str += n;
    sprintf(str, "\tmethods: %n", &n);
    str += n;

    ForAllAvailableToneMaps(method)
                {
                    if ( !first ) {
                        sprintf(str, "\t         %n", &n);
                        str += n;
                    }
                    first = false;
                    sprintf(str, "%-20.20s %s%s\n%n",
                            method->shortName, method->name,
                            tmopts.ToneMap == method ? " (default)" : "", &n);
                    str += n;
                }
    EndForAll;
    *(str - 1) = '\0';    /* discard last newline character */
}

static void ToneMappingMethodOption(void *value) {
    char *name = *(char **) value;

    ForAllAvailableToneMaps(method)
                {
                    if ( strncasecmp(name, method->shortName, method->abbrev) == 0 ) {
                        SetToneMap(method);
                        return;
                    }
                }
    EndForAll;

    Error(nullptr, "Invalid tone mapping method name '%s'", name);
}

/* tone mapping options */
static void BrightnessAdjustOption(void *val) {
    tmopts.pow_bright_adjust = pow(2., tmopts.brightness_adjust);
}

static float rxy[2], gxy[2], bxy[2], wxy[2];

static void ChromaOption(void *value) {
    float *chroma = (float *) value;
    if ( chroma == rxy ) {
        tmopts.xr = chroma[0];
        tmopts.yr = chroma[1];
    } else if ( chroma == gxy ) {
        tmopts.xg = chroma[0];
        tmopts.yg = chroma[1];
    } else if ( chroma == bxy ) {
        tmopts.xb = chroma[0];
        tmopts.yb = chroma[1];
    } else if ( chroma == wxy ) {
        tmopts.xw = chroma[0];
        tmopts.yw = chroma[1];
    } else {
        Fatal(-1, "ChromaOption", "invalid value pointer");
    }

    ComputeColorConversionTransforms(tmopts.xr, tmopts.yr,
                                     tmopts.xg, tmopts.yg,
                                     tmopts.xb, tmopts.yb,
                                     tmopts.xw, tmopts.yw);
}

static void _tmAdaptMethodOption(void *value) {
    char *name = *(char **) value;

    if ( strncasecmp(name, "average", 2) == 0 ) {
        tmopts.statadapt = TMA_AVERAGE;
    } else if ( strncasecmp(name, "median", 2) == 0 ) {
        tmopts.statadapt = TMA_MEDIAN;
        /* not yet supported
        else if (strncasecmp(name, "idrender", 2) == 0)
          tmopts.statadapt = TMA_IDRENDER;
        */
    } else {
        Error(nullptr,
              "Invalid adaptation estimate method '%s'",
              name);
    }
}

static void GammaOption(void *value) {
    float gam = *(float *) value;
    RGBSET(tmopts.gamma, gam, gam, gam);
}

static CMDLINEOPTDESC tmOptions[] = {
        {"-tonemapping",       4, Tstring,
                nullptr,                   ToneMappingMethodOption,
                tonemapping_methods_string},
        {"-brightness-adjust", 4, Tfloat,
                &tmopts.brightness_adjust, BrightnessAdjustOption,
                "-brightness-adjust <float> : brightness adjustment factor"},
        {"-adapt",             5, Tstring,
                nullptr,                   _tmAdaptMethodOption,
                "-adapt <method>  \t: adaptation estimation method\n"
                "\tmethods: \"average\", \"median\""},
        {"-lwa",               3, Tfloat,
                &tmopts.lwa,   DEFAULT_ACTION,
                "-lwa <float>\t\t: real world adaptation luminance"},
        {"-ldmax",             5, Tfloat,
                &tmopts.ldm,   DEFAULT_ACTION,
                "-ldmax <float>\t\t: maximum diaply luminance"},
        {"-cmax",              4, Tfloat,
                &tmopts.cmax,  DEFAULT_ACTION,
                "-cmax <float>\t\t: maximum displayable contrast"},
        {"-gamma",             4, Tfloat,
                nullptr,                   GammaOption,
                "-gamma <float>       \t: gamma correction factor (same for red, green. blue)"},
        {"-rgbgamma",          4, TRGB,
                &tmopts.gamma, DEFAULT_ACTION,
                "-rgbgamma <r> <g> <b>\t: gamma correction factor (separate for red, green, blue)"},
        {"-red",               4, Txy,
                rxy,                       ChromaOption,
                "-red <xy>            \t: CIE xy chromaticity of monitor red"},
        {"-green",             4, Txy,
                gxy,                       ChromaOption,
                "-green <xy>          \t: CIE xy chromaticity of monitor green"},
        {"-blue",              4, Txy,
                bxy,                       ChromaOption,
                "-blue <xy>           \t: CIE xy chromaticity of monitor blue"},
        {"-white",             4, Txy,
                wxy,                       ChromaOption,
                "-white <xy>          \t: CIE xy chromaticity of monitor white"},
        {nullptr,              0, TYPELESS,
                nullptr,       DEFAULT_ACTION,
                nullptr}
};

/* tone mapping defaults */
void ToneMapDefaults() {
    ForAllAvailableToneMaps(map)
                {
                    map->Defaults();
                }
    EndForAll;

    tmopts.brightness_adjust = 0.;
    tmopts.pow_bright_adjust = pow(2., tmopts.brightness_adjust);

    tmopts.statadapt = TMA_MEDIAN;
    tmopts.lwa = DEFAULT_TM_LWA;
    tmopts.ldm = DEFAULT_TM_LDMAX;
    tmopts.cmax = DEFAULT_TM_CMAX;

    rxy[0] = tmopts.xr = 0.640;
    rxy[1] = tmopts.yr = 0.330;
    gxy[0] = tmopts.xg = 0.290;
    gxy[1] = tmopts.yg = 0.600;
    bxy[0] = tmopts.xb = 0.150;
    bxy[1] = tmopts.yb = 0.060;
    wxy[0] = tmopts.xw = 0.333333333333;
    wxy[1] = tmopts.yw = 0.333333333333;
    ComputeColorConversionTransforms(tmopts.xr, tmopts.yr,
                                     tmopts.xg, tmopts.yg,
                                     tmopts.xb, tmopts.yb,
                                     tmopts.xw, tmopts.yw);

    RGBSET(tmopts.gamma, DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA);
    RecomputeGammaTables(tmopts.gamma);
    tmopts.ToneMap = &TM_Lightness;
    tmopts.ToneMap->Init();

    make_tonemapping_methods_string();
}

void ParseToneMapOptions(int *argc, char **argv) {
    ParseOptions(tmOptions, argc, argv);
    RecomputeGammaTables(tmopts.gamma);

    ForAllAvailableToneMaps(map)
                {
                    if ( map->ParseOptions ) {
                        map->ParseOptions(argc, argv);
                    }
                }
    EndForAll;
}

/* makes map the current tone mapping operator + initialises */
void SetToneMap(TONEMAP *map) {
    tmopts.ToneMap->Terminate();
    tmopts.ToneMap = map ? map : &TM_Dummy;
    tmopts.ToneMap->Init();
}

/* initialises tone mapping, e.g. for a new scene */
void InitToneMapping() {
    InitSceneAdaptation();
    SetToneMap(tmopts.ToneMap);
}

void RecomputeGammaTable(int index, double gamma) {
    int i;
    if ( gamma <= EPSILON ) {
        gamma = 1.;
    }
    for ( i = 0; i <= (1 << GAMMATAB_BITS); i++ ) {
        tmopts.gammatab[index][i] = pow((double) i / (double) (1 << GAMMATAB_BITS), 1. / gamma);
    }
}

/* recomputes gamma tables for the given gamma values for 
 * red, green and blue */
void RecomputeGammaTables(RGB gamma) {
    RecomputeGammaTable(0, gamma.r);
    RecomputeGammaTable(1, gamma.g);
    RecomputeGammaTable(2, gamma.b);
}

/* ---------------------------------------------------------------------------
  `RescaleRadiance'

  Rescale real world radiance using appropiately set up tone mapping
  algorithm.
  ------------------------------------------------------------------------- */
COLOR *RescaleRadiance(COLOR in, COLOR *out) {
    COLORSCALE(tmopts.pow_bright_adjust, in, in);
    *out = TonemapScaleForDisplay(in);
    return out;
}

RGB *RadianceToRGB(COLOR color, RGB *rgb) {
    RescaleRadiance(color, &color);
    ColorToRGB(color, rgb);
    RGBCLIP(*rgb);
    return rgb;
}
