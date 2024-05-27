#include "tonemap/WardToneMap.h"

/**
References:

G. Ward. A Contrast-Based Scale factor for Luminance Display, Graphics
Gems IV, Academic Press, 1994, pp. 415-421.
*/
static float globalComp;
static float globalDisp;
static float globalLda;

static void
wardDefaults() {
}

static void
wardTerminate() {
}

static void
wardInit() {
    float lwa = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float ldmax = GLOBAL_toneMap_options.maximumDisplayLuminance;
    globalLda = ldmax / 2.0f;

    float p1 = java::Math::pow(globalLda, 0.4f);
    float p2 = java::Math::pow(lwa, 0.4f);
    float p3 = (1.219f + p1) / (1.219f + p2);
    globalComp = java::Math::pow(p3, 2.5f);
    globalDisp = globalComp / ldmax;
}

static ColorRgb
wardScaleForComputations(ColorRgb radiance) {
    radiance.scale(globalComp);
    return radiance;
}

static ColorRgb
wardScaleForDisplay(ColorRgb radiance) {
    float eff = getLuminousEfficacy();

    radiance.scale(eff * globalDisp);
    return radiance;
}

ToneMap GLOBAL_toneMap_ward = {
    "Ward's Mapping",
    "Ward",
    3,
    wardDefaults,
    nullptr,
    wardInit,
    wardTerminate,
    wardScaleForComputations,
    wardScaleForDisplay
};
