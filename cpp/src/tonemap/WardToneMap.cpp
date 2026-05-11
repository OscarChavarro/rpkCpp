#include "common/color/Cie.h"
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
    const float realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
    const float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    lda = maximumDisplayLuminance / 2.0F;

    const float p1 = java::Math::pow(lda, 0.4F);
    const float p2 = java::Math::pow(realWorldAdaptionLuminance, 0.4F);
    const float p3 = (1.219F + p1) / (1.219F + p2);
    comp = java::Math::pow(p3, 2.5F);
    display = comp / maximumDisplayLuminance;
}

ColorRgb
WardToneMap::scaleForComputations(ColorRgb radiance) const {
    radiance.scale(comp);
    return radiance;
}

ColorRgb
WardToneMap::scaleForDisplay(ColorRgb radiance) const {
    const float eff = Cie::getLuminousEfficacy();

    radiance.scale(eff * display);
    return radiance;
}
