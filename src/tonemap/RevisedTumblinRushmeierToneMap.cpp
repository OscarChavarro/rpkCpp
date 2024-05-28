#include "java/lang/Math.h"
#include "tonemap/RevisedTumblinRushmeierToneMap.h"

/**
References:

[TUMB1999b] J. Tumblin, J.K. Hodgins, B.K. Guenter. Two Methods for Display of High
Contrast Images, ACM Transactions on Graphics, 18:1, 1999, pp. 56-94.
*/

static float globalG;
static float globalComp;
static float globalDisp;
static float globalLwa;
static float globalLdaTumb;

RevisedTumblinRushmeierToneMap::RevisedTumblinRushmeierToneMap() {
}

RevisedTumblinRushmeierToneMap::~RevisedTumblinRushmeierToneMap() {
}

void
RevisedTumblinRushmeierToneMap::init() {
    float lwa = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float ldmax = GLOBAL_toneMap_options.maximumDisplayLuminance;
    float cmax = GLOBAL_toneMap_options.maximumDisplayContrast;
    globalLdaTumb = ldmax / java::Math::sqrt(cmax);

    globalG = stevensGamma(lwa) / stevensGamma(globalLdaTumb);
    float gwd = stevensGamma(lwa) / (1.855f + 0.4f * java::Math::log(globalLdaTumb));
    globalComp = java::Math::pow(java::Math::sqrt(cmax), gwd - 1) * globalLdaTumb;
    globalDisp = globalComp / ldmax;
}

float
RevisedTumblinRushmeierToneMap::stevensGamma(float lum) {
    if ( lum > 100.0 ) {
        return 2.655f;
    } else {
        return 1.855f + 0.4f * java::Math::log10(lum + 2.3e-5f);
    }
}

static ColorRgb
revisedTRScaleForComputations(ColorRgb radiance) {
    float rwl;
    float scale;

    rwl = radiance.luminance();

    if ( rwl > 0.0 ) {
        scale = globalComp * java::Math::pow(rwl / globalLwa, globalG) / rwl;
    } else {
        scale = 0.0;
    }

    radiance.scale(scale);
    return radiance;
}

static ColorRgb
revisedTRScaleForDisplay(ColorRgb radiance) {
    float rwl = (float)M_PI * radiance.luminance();
    float eff = getLuminousEfficacy();
    radiance.scale(eff * (float)M_PI);

    float scale;
    if ( rwl > 0.0 ) {
        scale = globalDisp * java::Math::pow(rwl / globalLwa, globalG) / rwl;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}

OldToneMap GLOBAL_toneMap_revisedTumblinRushmeier = {
    "Revised Tumblin/Rushmeier's Mapping",
    "RevisedTR",
    3,
    nullptr,
    revisedTRScaleForComputations,
    revisedTRScaleForDisplay
};
