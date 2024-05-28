#include "java/lang/Math.h"
#include "tonemap/WardToneMap.h"

/**
References:

G. Ward. A Contrast-Based Scale factor for Luminance Display, Graphics
Gems IV, Academic Press, 1994, pp. 415-421.
*/
static float globalComp;
static float globalDisplay;
static float globalLda;

WardToneMap::WardToneMap() {
}

WardToneMap::~WardToneMap() {
}

void
WardToneMap::init() {
    float realWorldAdaptionLuminance = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float maximumDisplayLuminance = GLOBAL_toneMap_options.maximumDisplayLuminance;
    globalLda = maximumDisplayLuminance / 2.0f;

    float p1 = java::Math::pow(globalLda, 0.4f);
    float p2 = java::Math::pow(realWorldAdaptionLuminance, 0.4f);
    float p3 = (1.219f + p1) / (1.219f + p2);
    globalComp = java::Math::pow(p3, 2.5f);
    globalDisplay = globalComp / maximumDisplayLuminance;
}

ColorRgb
WardToneMap::scaleForComputations(ColorRgb radiance) const {
    radiance.scale(globalComp);
    return radiance;
}

ColorRgb
WardToneMap::scaleForDisplay(ColorRgb radiance) const {
    float eff = getLuminousEfficacy();

    radiance.scale(eff * globalDisplay);
    return radiance;
}
