#include "java/lang/Math.h"
#include "tonemap/RevisedTumblinRushmeierToneMap.h"

/**
References:

[TUMB1999b] J. Tumblin, J.K. Hodgins, B.K. Guenter. Two Methods for Display of High
Contrast Images, ACM Transactions on Graphics, 18:1, 1999, pp. 56-94.
*/

static float globalG;
static float globalComp;
static float globalDisplay;
static float globalLwa;
static float globalLdaTumblin;

RevisedTumblinRushmeierToneMap::RevisedTumblinRushmeierToneMap() {
}

RevisedTumblinRushmeierToneMap::~RevisedTumblinRushmeierToneMap() {
}

void
RevisedTumblinRushmeierToneMap::init() {
    float lwa = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float ldMax = GLOBAL_toneMap_options.maximumDisplayLuminance;
    float maximumDisplayContrast = GLOBAL_toneMap_options.maximumDisplayContrast;
    globalLdaTumblin = ldMax / java::Math::sqrt(maximumDisplayContrast);

    globalG = stevensGamma(lwa) / stevensGamma(globalLdaTumblin);
    float gwd = stevensGamma(lwa) / (1.855f + 0.4f * java::Math::log(globalLdaTumblin));
    globalComp = java::Math::pow(java::Math::sqrt(maximumDisplayContrast), gwd - 1) * globalLdaTumblin;
    globalDisplay = globalComp / ldMax;
}

ColorRgb
RevisedTumblinRushmeierToneMap::scaleForComputations(ColorRgb radiance) const {
    float rwl = radiance.luminance();
    float scale;

    if ( rwl > 0.0 ) {
        scale = globalComp * java::Math::pow(rwl / globalLwa, globalG) / rwl;
    } else {
        scale = 0.0;
    }

    radiance.scale(scale);
    return radiance;
}

ColorRgb
RevisedTumblinRushmeierToneMap::scaleForDisplay(ColorRgb radiance) const {
    float rwl = (float)M_PI * radiance.luminance();
    float eff = getLuminousEfficacy();
    radiance.scale(eff * (float)M_PI);

    float scale;
    if ( rwl > 0.0 ) {
        scale = globalDisplay * java::Math::pow(rwl / globalLwa, globalG) / rwl;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}

float
RevisedTumblinRushmeierToneMap::stevensGamma(float lum) {
    if ( lum > 100.0 ) {
        return 2.655f;
    } else {
        return 1.855f + 0.4f * java::Math::log10(lum + 2.3e-5f);
    }
}
