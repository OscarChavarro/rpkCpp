#include "java/lang/Math.h"
#include "vsdk/toolkit/tonemap/FerwerdaToneMap.h"

/**
References:
[FERW1996] J.A. Ferwerda, S.N. Pattanaik, P. Shirley, D. Greenberg. A Model of
Visual Adaptation for Realistic Image Synthesis, SIGGRAPH 1996,
pp. 249-258.
*/

FerwerdaToneMap::FerwerdaToneMap():
    sf(0.062, 0.608, 0.330),
    msf(),
    pmComp(),
    pmDisplay(),
    smComp(),
    smDisplay(),
    lda()
{
}

FerwerdaToneMap::~FerwerdaToneMap() {
}

void
FerwerdaToneMap::initParameters(const ToneMappingContext &toneMapOptions) {
    const double realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
    const double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    lda = maximumDisplayLuminance / 2.0;

    // Equations [FERW1996](4) and [FERW1996](5): t_p(L_a), t_s(L_a)
    msf = FerwerdaToneMap::mesopicScaleFactor(java::Math::log10(realWorldAdaptionLuminance));
    // Equation [FERW1996](3): m = t(L_da) / t(L_wa) using scotopic t_s from Equation (5)
    smComp = FerwerdaToneMap::scotopicOperator(java::Math::log10(lda)) /
             FerwerdaToneMap::scotopicOperator(java::Math::log10(realWorldAdaptionLuminance));
    // Equation [FERW1996](3): m = t(L_da) / t(L_wa) using photopic t_p from Equation (4)
    pmComp = FerwerdaToneMap::photopicOperator(java::Math::log10(lda)) /
             FerwerdaToneMap::photopicOperator(java::Math::log10(realWorldAdaptionLuminance));
    smDisplay = smComp / maximumDisplayLuminance;
    pmDisplay = pmComp / maximumDisplayLuminance;
}

ColorRgb
FerwerdaToneMap::scaleForComputations(const ColorRgb &radiance) const {
    // Convert to photometric values and compute the scotopic grayscale shift
    const double eff = getColorSpace().getLuminousEfficacy();
    const ColorRgb p = radiance.multiply(eff);
    double sl;
    // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
    sl = static_cast<double>(smComp * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB()));

    // Scale the photopic luminance
    const ColorRgb photopic = p.multiply(pmComp);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        return ColorRgb(photopic.getR() + sl, photopic.getG() + sl, photopic.getB() + sl);
    }

    return photopic;
}

ColorRgb
FerwerdaToneMap::scaleForDisplay(const ColorRgb &/*radiance*/) const {
    const ColorRgb p;
    double sl;

    // The legacy display path uses the zero-initialized scotopic color.
    // Keep this behavior while changing the color value type.
    // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
    sl = static_cast<double>(smDisplay * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB()));

    // Scale the photopic luminance
    const ColorRgb photopic = p.multiply(pmDisplay);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        return ColorRgb(photopic.getR() + sl, photopic.getG() + sl, photopic.getB() + sl);
    }

    return photopic;
}

double
FerwerdaToneMap::photopicOperator(double logLa) {
    // Equation [FERW1996](4): piecewise approximation of log(t_p(L_a))
    double r;
    if ( logLa <= -2.6 ) {
        r = -0.72;
    } else if ( logLa >= 1.9 ) {
        r = logLa - 1.255;
    } else {
        r = java::Math::pow(0.249 * logLa + 0.65, 2.7) - 0.72;
    }

    return java::Math::pow(10.0, r);
}

double
FerwerdaToneMap::scotopicOperator(double logLa) {
    // Equation [FERW1996](5): piecewise approximation of log(t_s(L_a))
    double r;
    if ( logLa <= -3.94 ) {
        r = -2.86;
    } else if ( logLa >= -1.44 ) {
            r = logLa - 0.395;
        } else {
            r = java::Math::pow(0.405 * logLa + 1.6, 2.18) - 2.86;
        }

    return java::Math::pow(10.0, r);
}

double
FerwerdaToneMap::mesopicScaleFactor(double logLwa) {
    if ( logLwa < -2.5 ) {
        return 1.0;
    } else if ( logLwa > 0.8 ) {
            return 0.0;
        } else {
            return (0.8 - logLwa) / 3.3;
        }
}
