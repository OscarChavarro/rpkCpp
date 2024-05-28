#include "java/lang/Math.h"
#include "common/ColorRgb.h"
#include "common/cie.h"
#include "tonemap/TumblinRushmeierToneMap.h"

/**
References:

[TUMB1993] J. Tumblin, H.E. Rushmeier. Tone Reproduction for Realistic Images,
IEEE Computer Graphics and Applications, 13:6, 1993, pp. 42-48.
*/

static float globalInvcmax;
static float globalLrwmComp;
static float globalLrwmDisp;
static float globalLrwExponent;
static float globalLdaTumb;

TumblinRushmeierToneMap::TumblinRushmeierToneMap() {
}

TumblinRushmeierToneMap::~TumblinRushmeierToneMap() {
}

void
TumblinRushmeierToneMap::init() {
}

static void
tumblinRushmeierInit() {
    float lwa = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float ldmax = GLOBAL_toneMap_options.maximumDisplayLuminance;
    float cmax = GLOBAL_toneMap_options.maximumDisplayContrast;
    globalLdaTumb = ldmax / java::Math::sqrt(cmax);

    float l10 = java::Math::log10(tmoCandelaLambert(lwa));
    float alpharw = 0.4f * l10 + 2.92f;
    float betarw = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

    l10 = java::Math::log10(tmoCandelaLambert(globalLdaTumb));
    float alphad = 0.4f * l10 + 2.92f;
    float betad = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

    globalLrwExponent = alpharw / alphad;
    globalLrwmComp = java::Math::pow(10.0f, (betarw - betad) / alphad);
    globalLrwmDisp = globalLrwmComp / (tmoCandelaLambert(ldmax));
    globalInvcmax = 1.0f / cmax;
}

static ColorRgb
trwfScaleForComputations(ColorRgb radiance) {
    float rwl = radiance.luminance();

    float scale;
    if ( rwl > 0.0 ) {
        float m = tmoLambertCandela(
                java::Math::pow(tmoCandelaLambert(rwl), globalLrwExponent) * globalLrwmComp);
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}

static ColorRgb
trwfScaleForDisplay(ColorRgb radiance) {
    float rwl = (float)M_PI * radiance.luminance();
    float eff = getLuminousEfficacy();
    radiance.scale(eff * (float) M_PI);

    float scale;
    if ( rwl > 0.0 ) {
        float m = (java::Math::pow(tmoCandelaLambert(rwl), globalLrwExponent) * globalLrwmDisp - globalInvcmax);
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}

OldToneMap GLOBAL_toneMap_tumblinRushmeier = {
    "Tumblin/Rushmeier's Mapping",
    "TumblinRushmeier",
    3,
    nullptr,
    tumblinRushmeierInit,
    trwfScaleForComputations,
    trwfScaleForDisplay
};
