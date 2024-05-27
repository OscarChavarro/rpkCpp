#include "IMAGE/tonemap/ToneMappingContext.h"

// Tone mapping context
ToneMappingContext GLOBAL_toneMap_options;

ToneMappingContext::ToneMappingContext():
    brightness_adjust(),
    pow_bright_adjust(),
    toneMap(),
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
}
