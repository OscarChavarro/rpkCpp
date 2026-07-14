#include "vsdk/toolkit/common/color/Cie.h"
#include "vsdk/toolkit/tonemap/FerwerdaToneMap.h"

/**
References:
[FERW1996] J.A. Ferwerda, S.N. Pattanaik, P. Shirley, D. Greenberg. A Model of
Visual Adaptation for Realistic Image Synthesis, SIGGRAPH 1996,
pp. 249-258.
*/

FerwerdaToneMap::FerwerdaToneMap():
    sf(0.062F, 0.608F, 0.330F),
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
FerwerdaToneMap::init(const ToneMappingContext &toneMapOptions) {
    const float realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
    const float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
    lda = maximumDisplayLuminance / 2.0F;

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

ColorRgbMutable
FerwerdaToneMap::scaleForComputations(ColorRgbMutable radiance) const {
    ColorRgbMutable p{};
    float sl;

    // Convert to photometric values
    const float eff = Cie::getLuminousEfficacy();
    radiance.scale(eff);

    // Compute the scotopic grayscale shift
    p = ColorRgbMutable(radiance.getR(), radiance.getG(), radiance.getB());
    // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
    sl = static_cast<float>(smComp * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB()));

    // Scale the photopic luminance
    radiance.scale(pmComp);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        radiance.addConstant(radiance, sl);
    }

    return radiance;
}

ColorRgbMutable
FerwerdaToneMap::scaleForDisplay(ColorRgbMutable radiance) const {
    ColorRgbMutable p{};
    float sl;

    // Convert to photometric values
    const float eff = Cie::getLuminousEfficacy();
    radiance.scale(eff);

    // Compute the scotopic grayscale shift
    radiance = ColorRgbMutable(p.getR(), p.getG(), p.getB());
    // Equation [FERW1996](6): L_d = L_dp + k(L_a) * L_ds
    sl = static_cast<float>(smDisplay * msf * (p.getR() * sf.getR() + p.getG() * sf.getG() + p.getB() * sf.getB()));

    // Scale the photopic luminance
    radiance.scale(pmDisplay);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        radiance.addConstant(radiance, sl);
    }

    return radiance;
}

float
FerwerdaToneMap::photopicOperator(float logLa) {
    // Equation [FERW1996](4): piecewise approximation of log(t_p(L_a))
    float r;
    if ( logLa <= -2.6 ) {
        r = -0.72F;
    } else if ( logLa >= 1.9 ) {
        r = logLa - 1.255F;
    } else {
        r = java::Math::pow(0.249F * logLa + 0.65F, 2.7F) - 0.72F;
    }

    return java::Math::pow(10.0F, r);
}

float
FerwerdaToneMap::scotopicOperator(float logLa) {
    // Equation [FERW1996](5): piecewise approximation of log(t_s(L_a))
    float r;
    if ( logLa <= -3.94 ) {
        r = -2.86F;
    } else if ( logLa >= -1.44 ) {
            r = logLa - 0.395F;
        } else {
            r = java::Math::pow(0.405F * logLa + 1.6F, 2.18F) - 2.86F;
        }

    return java::Math::pow(10.0F, r);
}

float
FerwerdaToneMap::mesopicScaleFactor(float logLwa) {
    if ( logLwa < -2.5 ) {
        return 1.0;
    } else if ( logLwa > 0.8 ) {
            return 0.0;
        } else {
            return (0.8F - logLwa) / 3.3F;
        }
}
