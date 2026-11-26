#include "java/lang/Math.h"
#include "vsdk/toolkit/tonemap/WardToneMap.h"

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
WardToneMap::initParameters(const ToneMappingContext &toneMapOptions) {
    const double realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
    const double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    lda = maximumDisplayLuminance / 2.0;

    const double p1 = java::Math::pow(lda, 0.4);
    const double p2 = java::Math::pow(realWorldAdaptionLuminance, 0.4);
    const double p3 = (1.219 + p1) / (1.219 + p2);
    comp = java::Math::pow(p3, 2.5);
    display = comp / maximumDisplayLuminance;
}

ColorRgb
WardToneMap::scaleForComputations(const ColorRgb &radiance) const {
    return radiance.multiply(comp);
}

ColorRgb
WardToneMap::scaleForDisplay(const ColorRgb &radiance) const {
    const double eff = getColorSpace().getLuminousEfficacy();

    return radiance.multiply(eff * display);
}
