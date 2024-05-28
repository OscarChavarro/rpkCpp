#include "java/lang/Math.h"
#include "tonemap/ToneMappingContext.h"

// Tone mapping defaults
static const float DEFAULT_GAMMA = 1.7f;
static const float  DEFAULT_TM_LWA = 10.0f;
static const float  DEFAULT_TM_LD_MAXIMUM = 100.0f;
static const float  DEFAULT_TM_C_MAXIMUM = 50.0f;

// Tone mapping context
ToneMappingContext GLOBAL_toneMap_options;

ToneMappingContext::ToneMappingContext():
    brightness_adjust(),
    pow_bright_adjust(),
    selectedToneMap(),
    staticAdaptationMethod(),
    realWorldAdaptionLuminance(),
    maximumDisplayLuminance(),
    maximumDisplayContrast(),
    xr(),
    yr(),
    xg(),
    yg(),
    xb(),
    yb(),
    xw(),
    yw(),
    gamma(),
    gammaTab()
{
    brightness_adjust = 0.0;
    pow_bright_adjust = java::Math::pow(2.0f, brightness_adjust);

    staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    realWorldAdaptionLuminance = DEFAULT_TM_LWA;
    maximumDisplayLuminance = DEFAULT_TM_LD_MAXIMUM;
    maximumDisplayContrast = DEFAULT_TM_C_MAXIMUM;

    xr = 0.640f;
    yr = 0.330f;
    xg = 0.290f;
    yg = 0.600f;
    xb = 0.150f;
    yb = 0.060f;
    xw = 0.333333333333f;
    yw = 0.333333333333f;
    computeColorConversionTransforms(xr, yr, xg, yg, xb, yb, xw, yw);

    gamma.set(DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA);
    recomputeGammaTables(gamma);
}

ToneMappingContext::~ToneMappingContext() {
    if ( selectedToneMap != nullptr ) {
        delete selectedToneMap;
    }
}
