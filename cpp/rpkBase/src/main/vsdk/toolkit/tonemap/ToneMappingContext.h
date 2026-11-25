#ifndef __TONE_MAPPING_CONTEXT__
#define __TONE_MAPPING_CONTEXT__

#include "vsdk/toolkit/common/color/Cie.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
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

    // Display RGB mapping (corrects display non-linear response)
    ColorRgb gamma; // Gamma factors for red, green, blue
    float gammaTab[3][GAMMA_TABLE_SIZE]; // Gamma correction tables for red, green and blue

    ToneMappingContext();
    ~ToneMappingContext();

    // Conversion from radiance (COLOR type) to display RGB: monitor primary
    // colors and white point
    const Cie &getColorSpace() const { return colorSpace; }
    void setColorSpace(const Cie &inColorSpace) { colorSpace = inColorSpace; }

  private:
    Cie colorSpace;

    static constexpr float DEFAULT_GAMMA = 1.7F;
    static constexpr float DEFAULT_TM_LWA = 10.0F;
    static constexpr float DEFAULT_TM_LD_MAXIMUM = 100.0F;
    static constexpr float DEFAULT_TM_C_MAXIMUM = 50.0F;
};

#endif
