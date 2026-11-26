#include "java/lang/Math.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"

ToneMappingContext::ToneMappingContext():
    brightness_adjust(),
    pow_bright_adjust(),
    staticAdaptationMethod(),
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
    pow_bright_adjust = java::Math::pow(2.0, brightness_adjust);

    staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    realWorldAdaptionLuminance = DEFAULT_TM_LWA;
    maximumDisplayLuminance = DEFAULT_TM_LD_MAXIMUM;
    maximumDisplayContrast = DEFAULT_TM_C_MAXIMUM;

    gamma = ColorRgb(DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA);
    ToneMap::recomputeGammaTables(*this, gamma);
}

ToneMappingContext::~ToneMappingContext() {
}
