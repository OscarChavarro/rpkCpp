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
    double realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
    double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    lda = maximumDisplayLuminance / 2.0;

    // Equations [FERW1996](4) and [FERW1996](5): t_p(L_a), t_s(L_a)
    msf = FerwerdaToneMap::mesopicScaleFactor(Math::log10(realWorldAdaptionLuminance));
    // Equation [FERW1996](3): m = t(L_da) / t(L_wa) using scotopic t_s from Equation (5)
    smComp = FerwerdaToneMap::scotopicOperator(Math::log10(lda)) /
             FerwerdaToneMap::scotopicOperator(Math::log10(realWorldAdaptionLuminance));
    // Equation [FERW1996](3): m = t(L_da) / t(L_wa) using photopic t_p from Equation (4)
    pmComp = FerwerdaToneMap::photopicOperator(Math::log10(lda)) /
             FerwerdaToneMap::photopicOperator(Math::log10(realWorldAdaptionLuminance));
    smDisplay = smComp / maximumDisplayLuminance;
    pmDisplay = pmComp / maximumDisplayLuminance;
}

ColorRgb
FerwerdaToneMap::scaleForComputations(ColorRgb radiance) const {
    ColorRgb p = ColorRgb(0.0, 0.0, 0.0);
    double sl;

    // Convert to photometric values
    double eff = getColorSpace().getLuminousEfficacy();
    radiance = ColorRgb(radiance.getR() * eff, radiance.getG() * eff, radiance.getB() * eff);

    // Compute the scotopic grayscale shift
    p = ColorRgb(radiance.getR(), radiance.getG(), radiance.getB());
    // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
    sl = smComp * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB());

    // Scale the photopic luminance
    radiance = ColorRgb(radiance.getR() * pmComp, radiance.getG() * pmComp, radiance.getB() * pmComp);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        radiance = ColorRgb(radiance.getR() + sl, radiance.getG() + sl, radiance.getB() + sl);
    }

    return radiance;
}

ColorRgb
FerwerdaToneMap::scaleForDisplay(ColorRgb radiance) const {
    ColorRgb p = ColorRgb(0.0, 0.0, 0.0);
    double sl;

    // Convert to photometric values
    double eff = getColorSpace().getLuminousEfficacy();
    radiance = ColorRgb(radiance.getR() * eff, radiance.getG() * eff, radiance.getB() * eff);

    // Compute the scotopic grayscale shift
    radiance = ColorRgb(p.getR(), p.getG(), p.getB());
    // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
    sl = smDisplay * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB());

    // Scale the photopic luminance
    radiance = ColorRgb(radiance.getR() * pmDisplay, radiance.getG() * pmDisplay, radiance.getB() * pmDisplay);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        radiance = ColorRgb(radiance.getR() + sl, radiance.getG() + sl, radiance.getB() + sl);
    }

    return radiance;
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
        r = Math::pow(0.249 * logLa + 0.65, 2.7) - 0.72;
    }

    return Math::pow(10.0, r);
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
            r = Math::pow(0.405 * logLa + 1.6, 2.18) - 2.86;
        }

    return Math::pow(10.0, r);
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
