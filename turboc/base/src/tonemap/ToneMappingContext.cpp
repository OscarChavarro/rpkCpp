#include "common/color/Cie.h"
#include "tonemap/ToneMappingContext.h"
#include "tonemap/ToneMap.h"

const float ToneMappingContext::DEFAULT_GAMMA = ((float)1.7);
const float ToneMappingContext::DEFAULT_TM_LWA = ((float)10.0);
const float ToneMappingContext::DEFAULT_TM_LD_MAXIMUM = ((float)100.0);
const float ToneMappingContext::DEFAULT_TM_C_MAXIMUM = ((float)50.0);

ToneMappingContext::ToneMappingContext():
    brightness_adjust(),
    pow_bright_adjust(),
    staticAdaptationMethod(TMA_MEDIAN),
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
    pow_bright_adjust = Math::pow(2.0f, brightness_adjust);

    realWorldAdaptionLuminance = ToneMappingContext::DEFAULT_TM_LWA;
    maximumDisplayLuminance = ToneMappingContext::DEFAULT_TM_LD_MAXIMUM;
    maximumDisplayContrast = ToneMappingContext::DEFAULT_TM_C_MAXIMUM;

    xr = 0.640f;
    yr = 0.330f;
    xg = 0.290f;
    yg = 0.600f;
    xb = 0.150f;
    yb = 0.060f;
    xw = 0.333333333333f;
    yw = 0.333333333333f;
    Cie::cmptClrConvXforms(xr, yr, xg, yg, xb, yb, xw, yw);

    gamma = ColorRgb(ToneMappingContext::DEFAULT_GAMMA, ToneMappingContext::DEFAULT_GAMMA, ToneMappingContext::DEFAULT_GAMMA);
    ToneMap::recomputeGammaTables(*this, gamma);
}

ToneMappingContext::~ToneMappingContext() {
}
