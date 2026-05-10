#include "common/color/Cie.h"
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
TumblinRushmeierToneMap::init(const ToneMappingContext &toneMapOptions) {
    const float lwa = toneMapOptions.realWorldAdaptionLuminance;
    const float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    const float maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    lda = maximumDisplayLuminance / java::Math::sqrt(maximumDisplayContrast);

    // Equation [COHE1993](9.16): alpha(L_w), beta(L_w) (Tumblin/Rushmeier model)
    float l10 = java::Math::log10(tmoCandelaLambert(lwa));
    const float alpha = 0.4f * l10 + 2.92f;
    const float beta = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

    // Equation [COHE1993](9.16): alpha(L_d), beta(L_d)
    l10 = java::Math::log10(tmoCandelaLambert(lda));
    const float alphaD = 0.4f * l10 + 2.92f;
    const float betaD = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

    // Equation [COHE1993](9.18): L_d from L_w using adaptation-dependent exponent and scale
    lrwExponent = alpha / alphaD;
    lrwmComp = java::Math::pow(10.0f, (beta - betaD) / alphaD);
    lrwmDisplay = lrwmComp / (tmoCandelaLambert(maximumDisplayLuminance));
    invCMaximum = 1.0f / maximumDisplayContrast;
}

ColorRgb
TumblinRushmeierToneMap::scaleForComputations(ColorRgb radiance) const {
    const float rwl = Cie::spectrumLuminance(radiance.r, radiance.g, radiance.b);

    float scale;
    if ( rwl > 0.0 ) {
        const float m = tmoLambertCandela(
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
    const float rwl = static_cast<float>(M_PI) * Cie::spectrumLuminance(radiance.r, radiance.g, radiance.b);
    const float eff = Cie::getLuminousEfficacy();
    radiance.scale(eff * static_cast<float>(M_PI));

    float scale;
    if ( rwl > 0.0 ) {
        const float m = (java::Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmDisplay - invCMaximum);
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}
