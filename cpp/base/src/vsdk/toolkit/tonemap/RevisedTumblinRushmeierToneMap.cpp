#include "vsdk/toolkit/common/color/Cie.h"
#include "vsdk/toolkit/tonemap/RevisedTumblinRushmeierToneMap.h"

/**
References:

[TUMB1999b] J. Tumblin, J.K. Hodgins, B.K. Guenter. Two Methods for Display of High
Contrast Images, ACM Transactions on Graphics, 18:1, 1999, pp. 56-94.
*/

RevisedTumblinRushmeierToneMap::RevisedTumblinRushmeierToneMap():
    g(),
    comp(),
    display(),
    lwaRTR(),
    ldaRTR()
{
}

RevisedTumblinRushmeierToneMap::~RevisedTumblinRushmeierToneMap() {
}

void
RevisedTumblinRushmeierToneMap::init(const ToneMappingContext &toneMapOptions) {
    const float lwa = toneMapOptions.realWorldAdaptionLuminance;
    const float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    const float maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    ldaRTR = maximumDisplayLuminance / java::Math::sqrt(maximumDisplayContrast);

    // Equation [TUMB1999b](17): exponent gw/gd
    g = stevensGamma(lwa) / stevensGamma(ldaRTR);
    // Equation [TUMB1999b](20): gwd = gw / (1.855 + 0.4 * log10(Lda))
    const float gwd = stevensGamma(lwa) / (1.855F + 0.4F * java::Math::log10(ldaRTR));
    // Equation [TUMB1999b](19): m(Lwa) * Lda
    comp = java::Math::pow(java::Math::sqrt(maximumDisplayContrast), gwd - 1) * ldaRTR;
    display = comp / maximumDisplayLuminance;
}

ColorRgb
RevisedTumblinRushmeierToneMap::scaleForComputations(ColorRgb radiance) const {
    const float rwl = Cie::spectrumLuminance(radiance.r, radiance.g, radiance.b);
    float scale;

    if ( rwl > 0.0 ) {
        // Equation [TUMB1999b](17) in multiplicative scale form
        scale = comp * java::Math::pow(rwl / lwaRTR, g) / rwl;
    } else {
        scale = 0.0;
    }

    radiance.scale(scale);
    return radiance;
}

ColorRgb
RevisedTumblinRushmeierToneMap::scaleForDisplay(ColorRgb radiance) const {
    const float rwl = static_cast<float>(M_PI) * Cie::spectrumLuminance(radiance.r, radiance.g, radiance.b);
    const float eff = Cie::getLuminousEfficacy();
    radiance.scale(eff * static_cast<float>(M_PI));

    float scale;
    if ( rwl > 0.0 ) {
        // Equation [TUMB1999b](17) in multiplicative scale form
        scale = display * java::Math::pow(rwl / lwaRTR, g) / rwl;
    } else {
        scale = 0.0F;
    }

    radiance.scale(scale);
    return radiance;
}

float
RevisedTumblinRushmeierToneMap::stevensGamma(float lum) {
    if ( lum > 100.0 ) {
        return 2.655F;
    } else {
        // Equation [TUMB1999b](18): g(L_a)
        return 1.855F + 0.4F * java::Math::log10(lum + 2.3e-5F);
    }
}
