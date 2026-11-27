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
    double lwa = toneMapOptions.realWorldAdaptionLuminance;
    double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    double maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
    lda = maximumDisplayLuminance / Math::sqrt(maximumDisplayContrast);

    // Equation [COHE1993](9.16): alpha(L_w), beta(L_w) (Tumblin/Rushmeier model)
    double l10 = Math::log10(tmoCandelaLambert(lwa));
    double alpha = 0.4 * l10 + 2.92;
    double beta = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;

    // Equation [COHE1993](9.16): alpha(L_d), beta(L_d)
    l10 = Math::log10(tmoCandelaLambert(lda));
    double alphaD = 0.4 * l10 + 2.92;
    double betaD = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;

    // Equation [COHE1993](9.18): L_d from L_w using adaptation-dependent exponent and scale
    lrwExponent = alpha / alphaD;
    lrwmComp = Math::pow(10.0, (beta - betaD) / alphaD);
    lrwmDisplay = lrwmComp / (tmoCandelaLambert(maximumDisplayLuminance));
    invCMaximum = 1.0 / maximumDisplayContrast;
}

ColorRgb
TumblinRushmeierToneMap::scaleForComputations(ColorRgb radiance) const {
    double rwl = getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());

    double scale;
    if ( rwl > 0.0 ) {
        double m = tmoLambertCandela(
                Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmComp);
        scale = m > 0.0 ? m / rwl : 0.0;
    } else {
        scale = 0.0;
    }

    return ColorRgb(
        radiance.getR() * scale,
        radiance.getG() * scale,
        radiance.getB() * scale);
}

ColorRgb
TumblinRushmeierToneMap::scaleForDisplay(ColorRgb radiance) const {
    double rwl = ((double)(M_PI)) * getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
    double eff = getColorSpace().getLuminousEfficacy();
    radiance = ColorRgb(
        radiance.getR() * eff * ((double)(M_PI)),
        radiance.getG() * eff * ((double)(M_PI)),
        radiance.getB() * eff * ((double)(M_PI)));

    double scale;
    if ( rwl > 0.0 ) {
        double m = (Math::pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmDisplay - invCMaximum);
        scale = m > 0.0 ? m / rwl : 0.0;
    } else {
        scale = 0.0;
    }

    return ColorRgb(
        radiance.getR() * scale,
        radiance.getG() * scale,
        radiance.getB() * scale);
}
