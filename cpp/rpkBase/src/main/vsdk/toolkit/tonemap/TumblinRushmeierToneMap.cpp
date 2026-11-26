#include "java/lang/Math.h"
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
TumblinRushmeierToneMap::initParameters(const ToneMappingContext &toneMapOptions) {
    const double lwa = toneMapOptions.realWorldAdaptionLuminance;
    const double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    const double maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    lda = maximumDisplayLuminance / java::Math::sqrt(maximumDisplayContrast);

    // Equation [COHE1993](9.16): alpha(L_w), beta(L_w) (Tumblin/Rushmeier model)
    double l10 = java::Math::log10(tmoCandelaLambert(lwa));
    const double alpha = 0.4 * l10 + 2.92;
    const double beta = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;

    // Equation [COHE1993](9.16): alpha(L_d), beta(L_d)
    l10 = java::Math::log10(tmoCandelaLambert(lda));
    const double alphaD = 0.4 * l10 + 2.92;
    const double betaD = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;

    // Equation [COHE1993](9.18): L_d from L_w using adaptation-dependent exponent and scale
    lrwExponent = alpha / alphaD;
    lrwmComp = java::Math::pow(10.0, (beta - betaD) / alphaD);
    lrwmDisplay = lrwmComp / (tmoCandelaLambert(maximumDisplayLuminance));
    invCMaximum = 1.0 / maximumDisplayContrast;
}

ColorRgb
TumblinRushmeierToneMap::scaleForComputations(const ColorRgb &radiance) const {
    const double rwl = static_cast<double>(getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB()));

    double scale;
    if ( rwl > 0.0 ) {
        const double m = tmoLambertCandela(
                java::Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmComp);
        scale = m > 0.0 ? m / rwl : 0.0;
    } else {
        scale = 0.0;
    }

    return radiance.multiply(scale);
}

ColorRgb
TumblinRushmeierToneMap::scaleForDisplay(const ColorRgb &radiance) const {
    const double rwl = static_cast<double>(M_PI) * static_cast<double>(getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB()));
    const double eff = static_cast<double>(getColorSpace().getLuminousEfficacy());
    const ColorRgb photometric = radiance.multiply(eff * static_cast<double>(M_PI));

    double scale;
    if ( rwl > 0.0 ) {
        const double m = (java::Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmDisplay - invCMaximum);
        scale = m > 0.0 ? m / rwl : 0.0;
    } else {
        scale = 0.0;
    }

    return photometric.multiply(scale);
}
