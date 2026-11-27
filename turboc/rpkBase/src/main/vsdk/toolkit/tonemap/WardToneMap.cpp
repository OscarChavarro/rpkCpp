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
    double realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
    double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    lda = maximumDisplayLuminance / 2.0;

    double p1 = Math::pow(lda, 0.4);
    double p2 = Math::pow(realWorldAdaptionLuminance, 0.4);
    double p3 = (1.219 + p1) / (1.219 + p2);
    comp = Math::pow(p3, 2.5);
    display = comp / maximumDisplayLuminance;
}

ColorRgb
WardToneMap::scaleForComputations(ColorRgb radiance) const {
    return ColorRgb(
        radiance.getR() * comp,
        radiance.getG() * comp,
        radiance.getB() * comp);
}

ColorRgb
WardToneMap::scaleForDisplay(ColorRgb radiance) const {
    double eff = getColorSpace().getLuminousEfficacy();
    return ColorRgb(
        radiance.getR() * eff * display,
        radiance.getG() * eff * display,
        radiance.getB() * eff * display);
}
