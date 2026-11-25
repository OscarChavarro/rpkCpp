#include "java/lang/Math.h"
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
RevisedTumblinRushmeierToneMap::initParameters(const ToneMappingContext &toneMapOptions) {
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
RevisedTumblinRushmeierToneMap::scaleForComputations(const ColorRgb &radiance) const {
    const float rwl = static_cast<float>(getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB()));
    float scale;

    if ( rwl > 0.0 ) {
        // Equation [TUMB1999b](17) in multiplicative scale form
        scale = comp * java::Math::pow(rwl / lwaRTR, g) / rwl;
    } else {
        scale = 0.0;
    }

    return radiance.multiply(scale);
}

ColorRgb
RevisedTumblinRushmeierToneMap::scaleForDisplay(const ColorRgb &radiance) const {
    const float rwl = static_cast<float>(M_PI) * static_cast<float>(getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB()));
    const float eff = static_cast<float>(getColorSpace().getLuminousEfficacy());
    const ColorRgb photometric = radiance.multiply(eff * static_cast<float>(M_PI));

    float scale;
    if ( rwl > 0.0 ) {
        // Equation [TUMB1999b](17) in multiplicative scale form
        scale = display * java::Math::pow(rwl / lwaRTR, g) / rwl;
    } else {
        scale = 0.0F;
    }

    return photometric.multiply(scale);
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
