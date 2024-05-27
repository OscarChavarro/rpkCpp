#ifndef __TONE_MAPPING_CONTEXT_H
#define __TONE_MAPPING_CONTEXT_H

#include "common/ColorRgb.h"
#include "tonemap/ToneMapAdaptationMethod.h"

class OldToneMap;
class ToneMap;

// Gamma correction table
#define GAMMA_TABLE_BITS 12
#define GAMMA_TABLE_SIZE ((1 << GAMMA_TABLE_BITS) + 1)

class ToneMappingContext {
  public:
    // Fixed radiance rescaling before tone mapping
    float brightness_adjust; // Brightness adjustment factor
    float pow_bright_adjust; // pow(2, brightness_adjust)

    // Variable / non-linear radiance rescaling
    OldToneMap *toneMap; // Current tone mapping operator
    ToneMap *selectedToneMap;
    ToneMapAdaptationMethod staticAdaptationMethod;
    float realWorldAdaptionLuminance;
    float maximumDisplayLuminance;
    float maximumDisplayContrast;

    // Conversion from radiance (COLOR type) to display RGB
    float xr; // Monitor primary colors
    float yr;
    float xg;
    float yg;
    float xb;
    float yb;
    float xw; // Monitor white point
    float yw;

    // Display RGB mapping (corrects display non-linear response)
    ColorRgb gamma; // Gamma factors for red, green, blue
    float gammaTab[3][GAMMA_TABLE_SIZE]; // Gamma correction tables for red, green and blue

    ToneMappingContext();
};
extern ToneMappingContext GLOBAL_toneMap_options;

#include "tonemap/ToneMap.h"

#endif
