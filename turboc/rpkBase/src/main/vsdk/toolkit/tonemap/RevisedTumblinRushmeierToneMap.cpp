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
    double lwa = toneMapOptions.realWorldAdaptionLuminance;
    double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    double maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    ldaRTR = maximumDisplayLuminance / Math::sqrt(maximumDisplayContrast);

    // Equation [TUMB1999b](17): exponent gw/gd
    g = stevensGamma(lwa) / stevensGamma(ldaRTR);
    // Equation [TUMB1999b](20): gwd = gw / (1.855 + 0.4 * log10(Lda))
    double gwd = stevensGamma(lwa) / (1.855 + 0.4 * Math::log10(ldaRTR));
    // Equation [TUMB1999b](19): m(Lwa) * Lda
    comp = Math::pow(Math::sqrt(maximumDisplayContrast), gwd - 1) * ldaRTR;
    display = comp / maximumDisplayLuminance;
}

ColorRgb
RevisedTumblinRushmeierToneMap::scaleForComputations(ColorRgb radiance) const {
    double rwl = getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
    double scale;

    if ( rwl > 0.0 ) {
        // Equation [TUMB1999b](17) in multiplicative scale form
        scale = comp * Math::pow(rwl / lwaRTR, g) / rwl;
    } else {
        scale = 0.0;
    }

    radiance = ColorRgb(radiance.getR() * scale, radiance.getG() * scale, radiance.getB() * scale);
    return radiance;
}

ColorRgb
RevisedTumblinRushmeierToneMap::scaleForDisplay(ColorRgb radiance) const {
    double rwl = ((double)(M_PI)) * getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
    double eff = getColorSpace().getLuminousEfficacy();
    radiance = ColorRgb(
        radiance.getR() * eff * ((double)(M_PI)),
        radiance.getG() * eff * ((double)(M_PI)),
        radiance.getB() * eff * ((double)(M_PI)));

    double scale;
    if ( rwl > 0.0 ) {
        // Equation [TUMB1999b](17) in multiplicative scale form
        scale = display * Math::pow(rwl / lwaRTR, g) / rwl;
    } else {
        scale = 0.0;
    }

    radiance = ColorRgb(radiance.getR() * scale, radiance.getG() * scale, radiance.getB() * scale);
    return radiance;
}

double
RevisedTumblinRushmeierToneMap::stevensGamma(double lum) {
    if ( lum > 100.0 ) {
        return 2.655;
    } else {
        // Equation [TUMB1999b](18): g(L_a)
        return 1.855 + 0.4 * Math::log10(lum + 2.3e-5);
    }
}
