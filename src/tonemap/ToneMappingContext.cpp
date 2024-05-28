#include "tonemap/ToneMappingContext.h"

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
}

ToneMappingContext::~ToneMappingContext() {
    if ( selectedToneMap != nullptr ) {
        delete selectedToneMap;
    }
}
