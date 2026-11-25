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
    // Monitor primaries and white point, given in single precision (the
    // command line options are single precision too)
    colorSpace(
        0.640F, 0.330F,
        0.290F, 0.600F,
        0.150F, 0.060F,
        0.333333333333F, 0.333333333333F)
{
    brightness_adjust = 0.0;
    pow_bright_adjust = java::Math::pow(2.0F, brightness_adjust);

    staticAdaptationMethod = ToneMapAdaptationMethod::TMA_MEDIAN;
    realWorldAdaptionLuminance = DEFAULT_TM_LWA;
    maximumDisplayLuminance = DEFAULT_TM_LD_MAXIMUM;
    maximumDisplayContrast = DEFAULT_TM_C_MAXIMUM;

    gamma = ColorRgb(DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA);
    ToneMap::recomputeGammaTables(*this, gamma);
}

ToneMappingContext::~ToneMappingContext() {
}
