#include "common/color/Cie.h"
#include "tonemap/ToneMappingContext.h"
#include "tonemap/ToneMap.h"

ToneMappingContext::ToneMappingContext():
    brightness_adjust(),
    pow_bright_adjust(),
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
    pow_bright_adjust = java::Math::pow(2.0F, brightness_adjust);

    staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    realWorldAdaptionLuminance = DEFAULT_TM_LWA;
    maximumDisplayLuminance = DEFAULT_TM_LD_MAXIMUM;
    maximumDisplayContrast = DEFAULT_TM_C_MAXIMUM;

    xr = 0.640F;
    yr = 0.330F;
    xg = 0.290F;
    yg = 0.600F;
    xb = 0.150F;
    yb = 0.060F;
    xw = 0.333333333333F;
    yw = 0.333333333333F;
    Cie::computeColorConversionTransforms(xr, yr, xg, yg, xb, yb, xw, yw);

    gamma.set(DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA);
    ToneMap::recomputeGammaTables(*this, gamma);
}

ToneMappingContext::~ToneMappingContext() {
}
