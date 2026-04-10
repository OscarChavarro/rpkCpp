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
WardToneMap::init(const ToneMappingContext &toneMapOptions) {
    float realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
    float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    lda = maximumDisplayLuminance / 2.0f;

    float p1 = Math::pow(lda, 0.4f);
    float p2 = Math::pow(realWorldAdaptionLuminance, 0.4f);
    float p3 = (1.219f + p1) / (1.219f + p2);
    comp = Math::pow(p3, 2.5f);
    display = comp / maximumDisplayLuminance;
}

ColorRgb
WardToneMap::scaleForComputations(ColorRgb radiance) const {
    radiance.scale(comp);
    return radiance;
}

ColorRgb
WardToneMap::scaleForDisplay(ColorRgb radiance) const {
    float eff = Cie::getLuminousEfficacy();

    radiance.scale(eff * display);
    return radiance;
}
