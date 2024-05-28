#include "java/lang/Math.h"
#include "common/ColorRgb.h"
#include "common/cie.h"
#include "tonemap/TumblinRushmeierToneMap.h"

/**
References:

[TUMB1993] J. Tumblin, H.E. Rushmeier. Tone Reproduction for Realistic Images,
IEEE Computer Graphics and Applications, 13:6, 1993, pp. 42-48.
*/

TumblinRushmeierToneMap::TumblinRushmeierToneMap():
    invCMaximum(),
    lrwmComp(),
    lrwmDisplay(),
    lrwExponent(),
    lda()
{
}

TumblinRushmeierToneMap::~TumblinRushmeierToneMap() {
}

void
TumblinRushmeierToneMap::init() {
    float lwa = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float maximumDisplayLuminance = GLOBAL_toneMap_options.maximumDisplayLuminance;
    float maximumDisplayContrast = GLOBAL_toneMap_options.maximumDisplayContrast;
    lda = maximumDisplayLuminance / java::Math::sqrt(maximumDisplayContrast);

    float l10 = java::Math::log10(tmoCandelaLambert(lwa));
    float alpha = 0.4f * l10 + 2.92f;
    float beta = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

    l10 = java::Math::log10(tmoCandelaLambert(lda));
    float alphaD = 0.4f * l10 + 2.92f;
    float betaD = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

    lrwExponent = alpha / alphaD;
    lrwmComp = java::Math::pow(10.0f, (beta - betaD) / alphaD);
    lrwmDisplay = lrwmComp / (tmoCandelaLambert(maximumDisplayLuminance));
    invCMaximum = 1.0f / maximumDisplayContrast;
}

ColorRgb
TumblinRushmeierToneMap::scaleForComputations(ColorRgb radiance) const {
    float rwl = radiance.luminance();

    float scale;
    if ( rwl > 0.0 ) {
        float m = tmoLambertCandela(
                java::Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmComp);
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}

ColorRgb
TumblinRushmeierToneMap::scaleForDisplay(ColorRgb radiance) const {
    float rwl = (float)M_PI * radiance.luminance();
    float eff = getLuminousEfficacy();
    radiance.scale(eff * (float) M_PI);

    float scale;
    if ( rwl > 0.0 ) {
        float m = (java::Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmDisplay - invCMaximum);
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}
