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
    const double lwa = toneMapOptions.realWorldAdaptionLuminance;
    const double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    const double maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    ldaRTR = maximumDisplayLuminance / java::Math::sqrt(maximumDisplayContrast);

    // Equation [TUMB1999b](17): exponent gw/gd
    g = stevensGamma(lwa) / stevensGamma(ldaRTR);
    // Equation [TUMB1999b](20): gwd = gw / (1.855 + 0.4 * log10(Lda))
    const double gwd = stevensGamma(lwa) / (1.855 + 0.4 * java::Math::log10(ldaRTR));
    // Equation [TUMB1999b](19): m(Lwa) * Lda
    comp = java::Math::pow(java::Math::sqrt(maximumDisplayContrast), gwd - 1) * ldaRTR;
    display = comp / maximumDisplayLuminance;
}

ColorRgb
RevisedTumblinRushmeierToneMap::scaleForComputations(const ColorRgb &radiance) const {
    const double rwl = static_cast<double>(getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB()));
    double scale;

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
    const double rwl = static_cast<double>(M_PI) * static_cast<double>(getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB()));
    const double eff = static_cast<double>(getColorSpace().getLuminousEfficacy());
    const ColorRgb photometric = radiance.multiply(eff * static_cast<double>(M_PI));

    double scale;
    if ( rwl > 0.0 ) {
        // Equation [TUMB1999b](17) in multiplicative scale form
        scale = display * java::Math::pow(rwl / lwaRTR, g) / rwl;
    } else {
        scale = 0.0;
    }

    return photometric.multiply(scale);
}

double
RevisedTumblinRushmeierToneMap::stevensGamma(double lum) {
    if ( lum > 100.0 ) {
        return 2.655;
    } else {
        // Equation [TUMB1999b](18): g(L_a)
        return 1.855 + 0.4 * java::Math::log10(lum + 2.3e-5);
    }
}
