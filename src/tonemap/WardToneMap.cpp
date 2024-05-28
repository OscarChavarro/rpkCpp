#include "java/lang/Math.h"
#include "tonemap/WardToneMap.h"

/**
References:

G. Ward. A Contrast-Based Scale factor for Luminance Display, Graphics
Gems IV, Academic Press, 1994, pp. 415-421.
*/

WardToneMap::WardToneMap(): comp(), display(), lda() {
}

WardToneMap::~WardToneMap() {
}

void
WardToneMap::init() {
    float realWorldAdaptionLuminance = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float maximumDisplayLuminance = GLOBAL_toneMap_options.maximumDisplayLuminance;
    lda = maximumDisplayLuminance / 2.0f;

    float p1 = java::Math::pow(lda, 0.4f);
    float p2 = java::Math::pow(realWorldAdaptionLuminance, 0.4f);
    float p3 = (1.219f + p1) / (1.219f + p2);
    comp = java::Math::pow(p3, 2.5f);
    display = comp / maximumDisplayLuminance;
}

ColorRgb
WardToneMap::scaleForComputations(ColorRgb radiance) const {
    radiance.scale(comp);
    return radiance;
}

ColorRgb
WardToneMap::scaleForDisplay(ColorRgb radiance) const {
    float eff = getLuminousEfficacy();

    radiance.scale(eff * display);
    return radiance;
}
