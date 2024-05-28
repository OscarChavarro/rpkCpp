#include "java/lang/Math.h"
#include "tonemap/RevisedTumblinRushmeierToneMap.h"

/**
References:

[TUMB1999b] J. Tumblin, J.K. Hodgins, B.K. Guenter. Two Methods for Display of High
Contrast Images, ACM Transactions on Graphics, 18:1, 1999, pp. 56-94.
*/

RevisedTumblinRushmeierToneMap::RevisedTumblinRushmeierToneMap():
    g(),
    comp(),
    display(),
    lwaRTR(),
    ldaRTR()
{
}

RevisedTumblinRushmeierToneMap::~RevisedTumblinRushmeierToneMap() {
}

void
RevisedTumblinRushmeierToneMap::init() {
    float lwa = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float maximumDisplayLuminance = GLOBAL_toneMap_options.maximumDisplayLuminance;
    float maximumDisplayContrast = GLOBAL_toneMap_options.maximumDisplayContrast;
    ldaRTR = maximumDisplayLuminance / java::Math::sqrt(maximumDisplayContrast);

    g = stevensGamma(lwa) / stevensGamma(ldaRTR);
    float gwd = stevensGamma(lwa) / (1.855f + 0.4f * java::Math::log(ldaRTR));
    comp = java::Math::pow(java::Math::sqrt(maximumDisplayContrast), gwd - 1) * ldaRTR;
    display = comp / maximumDisplayLuminance;
}

ColorRgb
RevisedTumblinRushmeierToneMap::scaleForComputations(ColorRgb radiance) const {
    float rwl = radiance.luminance();
    float scale;

    if ( rwl > 0.0 ) {
        scale = comp * java::Math::pow(rwl / lwaRTR, g) / rwl;
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
        scale = display * java::Math::pow(rwl / lwaRTR, g) / rwl;
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
