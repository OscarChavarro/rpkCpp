#include "java/lang/Math.h"
#include "common/ColorRgb.h"
#include "tonemap/FerwerdaToneMap.h"

/**
References:
[FERW1996] J.A. Ferwerda, S.N. Pattanaik, P. Shirley, D. Greenberg. A Model of
Visual Adaptation for Realistic Image Synthesis, SIGGRAPH 1996,
pp. 249-258.
*/

static ColorRgb globalSf(0.062f, 0.608f, 0.330f);
static float globalMsf;
static float globalPmComp;
static float globalPmDisp;
static float globalSmComp;
static float globalSmDisp;
static float globalLda;

FerwerdaToneMap::FerwerdaToneMap() {
}

FerwerdaToneMap::~FerwerdaToneMap() {
}

void
FerwerdaToneMap::init() {
    float realWorldAdaptionLuminance = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float maximumDisplayLuminance = GLOBAL_toneMap_options.maximumDisplayLuminance;
    globalLda = maximumDisplayLuminance / 2.0f;

    globalMsf = FerwerdaToneMap::mesopicScaleFactor(java::Math::log10(realWorldAdaptionLuminance));
    globalSmComp = FerwerdaToneMap::scotopicOperator(java::Math::log10(globalLda)) /
        FerwerdaToneMap::scotopicOperator(java::Math::log10(realWorldAdaptionLuminance));
    globalPmComp = FerwerdaToneMap::photopicOperator(java::Math::log10(globalLda)) /
        FerwerdaToneMap::photopicOperator(java::Math::log10(realWorldAdaptionLuminance));
    globalSmDisp = globalSmComp / maximumDisplayLuminance;
    globalPmDisp = globalPmComp / maximumDisplayLuminance;
}

float
FerwerdaToneMap::photopicOperator(float logLa) {
    float r;
    if ( logLa <= -2.6 ) {
        r = -0.72f;
    } else if ( logLa >= 1.9 ) {
        r = logLa - 1.255f;
    } else {
        r = java::Math::pow(0.249f * logLa + 0.65f, 2.7f) - 0.72f;
    }

    return java::Math::pow(10.0f, r);
}

float
FerwerdaToneMap::scotopicOperator(float logLa) {
    float r;
    if ( logLa <= -3.94 ) {
        r = -2.86f;
    } else if ( logLa >= -1.44 ) {
            r = logLa - 0.395f;
        } else {
            r = java::Math::pow(0.405f * logLa + 1.6f, 2.18f) - 2.86f;
        }

    return java::Math::pow(10.0f, r);
}

float
FerwerdaToneMap::mesopicScaleFactor(float logLwa) {
    if ( logLwa < -2.5 ) {
        return 1.0;
    } else if ( logLwa > 0.8 ) {
            return 0.0;
        } else {
            return (0.8f - logLwa) / 3.3f;
        }
}

static ColorRgb
ferwerdaScaleForComputations(ColorRgb radiance) {
    ColorRgb p{};
    float sl;

    // Convert to photometric values
    float eff = getLuminousEfficacy();
    radiance.scale(eff);

    // Compute the scotopic grayscale shift
    p.set(radiance.r, radiance.g, radiance.b);
    sl = globalSmComp * globalMsf * (p.r * globalSf.r + p.g * globalSf.g + p.b * globalSf.b);

    // Scale the photopic luminance
    radiance.scale(globalPmComp);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        radiance.addConstant(radiance, sl);
    }

    return radiance;
}

static ColorRgb
ferwerdaScaleForDisplay(ColorRgb radiance) {
    ColorRgb p{};
    float sl;

    // Convert to photometric values
    float eff = getLuminousEfficacy();
    radiance.scale(eff);

    // Compute the scotopic grayscale shift
    radiance.set(p.r, p.g, p.b);
    sl = globalSmDisp * globalMsf * (p.r * globalSf.r + p.g * globalSf.g + p.b * globalSf.b);

    // Scale the photopic luminance
    radiance.scale(globalPmDisp);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        radiance.addConstant(radiance, sl);
    }

    return radiance;
}

OldToneMap GLOBAL_toneMap_ferwerda = {
    "Partial Ferwerda's Mapping",
    "Ferwerda",
    3,
    nullptr,
    ferwerdaScaleForComputations,
    ferwerdaScaleForDisplay
};
