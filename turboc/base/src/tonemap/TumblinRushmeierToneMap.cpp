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
    float lwa = toneMapOptions.realWorldAdaptionLuminance;
    float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    float maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    lda = maximumDisplayLuminance / Math::sqrt(maximumDisplayContrast);

    // Equation [COHE1993](9.16): alpha(L_w), beta(L_w) (Tumblin/Rushmeier model)
    float l10 = Math::log10(tmoCandelaLambert(lwa));
    float alpha = 0.4f * l10 + 2.92f;
    float beta = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

    // Equation [COHE1993](9.16): alpha(L_d), beta(L_d)
    l10 = Math::log10(tmoCandelaLambert(lda));
    float alphaD = 0.4f * l10 + 2.92f;
    float betaD = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

    // Equation [COHE1993](9.18): L_d from L_w using adaptation-dependent exponent and scale
    lrwExponent = alpha / alphaD;
    lrwmComp = Math::pow(10.0f, (beta - betaD) / alphaD);
    lrwmDisplay = lrwmComp / (tmoCandelaLambert(maximumDisplayLuminance));
    invCMaximum = 1.0f / maximumDisplayContrast;
}

ColorRgb
TumblinRushmeierToneMap::scaleForComputations(ColorRgb radiance) const {
    float rwl = Cie::spectrumLuminance(radiance.r, radiance.g, radiance.b);

    float scale;
    if ( rwl > 0.0 ) {
        float m = tmoLambertCandela(
                Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmComp);
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}

ColorRgb
TumblinRushmeierToneMap::scaleForDisplay(ColorRgb radiance) const {
    float rwl = ((float)(M_PI)) * Cie::spectrumLuminance(radiance.r, radiance.g, radiance.b);
    float eff = Cie::getLuminousEfficacy();
    radiance.scale(eff * ((float)(M_PI)));

    float scale;
    if ( rwl > 0.0 ) {
        float m = (Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmDisplay - invCMaximum);
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}
