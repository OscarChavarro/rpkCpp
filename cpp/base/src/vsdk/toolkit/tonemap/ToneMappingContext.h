#ifndef TONE_MAPPING_CONTEXT_H
#define TONE_MAPPING_CONTEXT_H

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/tonemap/ToneMapAdaptationMethod.h"

class ToneMappingContext {
  public:
    // Gamma correction table
    static constexpr int GAMMA_TABLE_BITS = 12;
    static constexpr int GAMMA_TABLE_SIZE = (1 << GAMMA_TABLE_BITS) + 1;

    // Fixed radiance rescaling before tone mapping
    float brightness_adjust; // Brightness adjustment factor
    float pow_bright_adjust; // pow(2, brightness_adjust)

    // Variable / non-linear radiance rescaling parameters
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
    ColorRgbMutable gamma; // Gamma factors for red, green, blue
    float gammaTab[3][GAMMA_TABLE_SIZE]; // Gamma correction tables for red, green and blue

    ToneMappingContext();
    ~ToneMappingContext();

  private:
    static constexpr float DEFAULT_GAMMA = 1.7F;
    static constexpr float DEFAULT_TM_LWA = 10.0F;
    static constexpr float DEFAULT_TM_LD_MAXIMUM = 100.0F;
    static constexpr float DEFAULT_TM_C_MAXIMUM = 50.0F;
};

#endif
