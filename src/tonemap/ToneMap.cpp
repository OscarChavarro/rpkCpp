#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "common/linealAlgebra/Numeric.h"
#include "tonemap/IdentityToneMap.h"
#include "tonemap/LightnessToneMap.h"
#include "tonemap/WardToneMap.h"
#include "tonemap/TumblinRushmeierToneMap.h"
#include "tonemap/RevisedTumblinRushmeierToneMap.h"
#include "tonemap/FerwerdaToneMap.h"

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

    GLOBAL_toneMap_options.staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    GLOBAL_toneMap_options.realWorldAdaptionLuminance = DEFAULT_TM_LWA;
    GLOBAL_toneMap_options.maximumDisplayLuminance = DEFAULT_TM_LDMAX;
    GLOBAL_toneMap_options.maximumDisplayContrast = DEFAULT_TM_CMAX;

    GLOBAL_toneMap_options.xr = 0.640f;
    GLOBAL_toneMap_options.yr = 0.330f;
    GLOBAL_toneMap_options.xg = 0.290f;
    GLOBAL_toneMap_options.yg = 0.600f;
    GLOBAL_toneMap_options.xb = 0.150f;
    GLOBAL_toneMap_options.yb = 0.060f;
    GLOBAL_toneMap_options.xw = 0.333333333333f;
    GLOBAL_toneMap_options.yw = 0.333333333333f;
    computeColorConversionTransforms(GLOBAL_toneMap_options.xr, GLOBAL_toneMap_options.yr,
                                     GLOBAL_toneMap_options.xg, GLOBAL_toneMap_options.yg,
                                     GLOBAL_toneMap_options.xb, GLOBAL_toneMap_options.yb,
                                     GLOBAL_toneMap_options.xw, GLOBAL_toneMap_options.yw);

    GLOBAL_toneMap_options.gamma.set((float)DEFAULT_GAMMA, (float)DEFAULT_GAMMA, (float)DEFAULT_GAMMA);
    recomputeGammaTables(GLOBAL_toneMap_options.gamma);
    GLOBAL_toneMap_options.toneMap = &GLOBAL_toneMap_lightness;
    GLOBAL_toneMap_options.toneMap->Init();
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

void
recomputeGammaTable(int index, double gamma) {
    if ( gamma <= Numeric::EPSILON ) {
        gamma = 1.0;
    }
    for ( int i = 0; i <= (1 << GAMMA_TABLE_BITS); i++ ) {
        GLOBAL_toneMap_options.gammaTab[index][i] = (float)java::Math::pow((double) i / (double) (1 << GAMMA_TABLE_BITS), 1.0 / gamma);
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
