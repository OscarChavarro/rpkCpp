#include "java/lang/Math.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"

const double ToneMappingContext::DEFAULT_GAMMA = ((double)1.7);
const double ToneMappingContext::DEFAULT_TM_LWA = ((double)10.0);
const double ToneMappingContext::DEFAULT_TM_LD_MAXIMUM = ((double)100.0);
const double ToneMappingContext::DEFAULT_TM_C_MAXIMUM = ((double)50.0);

ToneMappingContext::ToneMappingContext():
    brightness_adjust(),
    pow_bright_adjust(),
    staticAdaptationMethod(TMA_MEDIAN),
    realWorldAdaptionLuminance(),
    maximumDisplayLuminance(),
    maximumDisplayContrast(),
    gamma(),
    gammaTab(),
    // Monitor primaries and white point
    colorSpace(
        0.640, 0.330,
        0.290, 0.600,
        0.150, 0.060,
        0.333333333333, 0.333333333333)
{
    brightness_adjust = 0.0;
    pow_bright_adjust = Math::pow(2.0, brightness_adjust);

    realWorldAdaptionLuminance = ToneMappingContext::DEFAULT_TM_LWA;
    maximumDisplayLuminance = ToneMappingContext::DEFAULT_TM_LD_MAXIMUM;
    maximumDisplayContrast = ToneMappingContext::DEFAULT_TM_C_MAXIMUM;

    gamma = ColorRgb(ToneMappingContext::DEFAULT_GAMMA, ToneMappingContext::DEFAULT_GAMMA, ToneMappingContext::DEFAULT_GAMMA);
    ToneMap::recomputeGammaTables(*this, gamma);
}

ToneMappingContext::~ToneMappingContext() {
}
