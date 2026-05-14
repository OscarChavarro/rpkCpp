#include "vsdk/toolkit/common/color/Cie.h"
#include "vsdk/toolkit/tonemap/TumblinRushmeierToneMap.h"

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
TumblinRushmeierToneMap::init(const ToneMappingContext &toneMapOptions) {
    const float lwa = toneMapOptions.realWorldAdaptionLuminance;
    const float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    const float maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    lda = maximumDisplayLuminance / java::Math::sqrt(maximumDisplayContrast);

    // Equation [COHE1993](9.16): alpha(L_w), beta(L_w) (Tumblin/Rushmeier model)
    float l10 = java::Math::log10(tmoCandelaLambert(lwa));
    const float alpha = 0.4F * l10 + 2.92F;
    const float beta = -0.4F * (l10 * l10) - 2.584F * l10 + 2.0208F;

    // Equation [COHE1993](9.16): alpha(L_d), beta(L_d)
    l10 = java::Math::log10(tmoCandelaLambert(lda));
    const float alphaD = 0.4F * l10 + 2.92F;
    const float betaD = -0.4F * (l10 * l10) - 2.584F * l10 + 2.0208F;

    // Equation [COHE1993](9.18): L_d from L_w using adaptation-dependent exponent and scale
    lrwExponent = alpha / alphaD;
    lrwmComp = java::Math::pow(10.0F, (beta - betaD) / alphaD);
    lrwmDisplay = lrwmComp / (tmoCandelaLambert(maximumDisplayLuminance));
    invCMaximum = 1.0F / maximumDisplayContrast;
}

ColorRgb
TumblinRushmeierToneMap::scaleForComputations(ColorRgb radiance) const {
    const float rwl = static_cast<float>(Cie::spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB()));

    float scale;
    if ( rwl > 0.0 ) {
        const float m = tmoLambertCandela(
                java::Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmComp);
        scale = m > 0.0F ? m / rwl : 0.0F;
    } else {
        scale = 0.0F;
    }

    radiance.scale(scale);
    return radiance;
}

ColorRgb
TumblinRushmeierToneMap::scaleForDisplay(ColorRgb radiance) const {
    const float rwl = static_cast<float>(M_PI) * static_cast<float>(Cie::spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB()));
    const float eff = static_cast<float>(Cie::getLuminousEfficacy());
    radiance.scale(eff * static_cast<float>(M_PI));

    float scale;
    if ( rwl > 0.0 ) {
        const float m = (java::Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmDisplay - invCMaximum);
        scale = m > 0.0F ? m / rwl : 0.0F;
    } else {
        scale = 0.0F;
    }

    radiance.scale(scale);
    return radiance;
}
