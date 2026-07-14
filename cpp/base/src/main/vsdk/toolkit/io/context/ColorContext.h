#ifndef COLOR_CONTEXT__
#define COLOR_CONTEXT__

#include "java/lang/Math.h"

enum {
    COLOR_MINIMUM_WAVE_LENGTH = 380,
    COLOR_MAXIMUM_WAVE_LENGTH = 780
};

enum {
    COLOR_SPECTRUM_IS_SET_FLAG = 01,
    COLOR_DEFINED_WITH_SPECTRUM_FLAG = 02,
    COLOR_XY_IS_SET_FLAG = 04,
    COLOR_DEFINED_WITH_XY_FLAG = 010,
    COLOR_EFFICACY_FLAG = 020
};

class ColorContext {
  public:
    static constexpr int NUMBER_OF_SPECTRAL_SAMPLES = 41; // Number of spectral samples
    static constexpr int COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE = 10000;
    static const ColorContext DEFAULT_COLOR_CONTEXT;

  private:
    // W-m^2
    static constexpr double C1 = 3.741832e-16;
    // m-K
    static constexpr double C2 = 1.4388e-2;
    static ColorContext cie_xp;
    static ColorContext cie_yp;
    static ColorContext cie_zp;
    static ColorContext cie_xf;
    static ColorContext cie_yf;
    static ColorContext cie_zf;

    inline static constexpr float
    colorWaveLengthDeltaI() {
        return static_cast<float>(COLOR_MAXIMUM_WAVE_LENGTH - COLOR_MINIMUM_WAVE_LENGTH) /
               static_cast<float>(NUMBER_OF_SPECTRAL_SAMPLES - 1);
    }

    inline static constexpr double
    colorPeakLumensPerWatt() {
        return 683.0 / COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE;
    }

    inline static double
    bBlm(double t) {
        return C2 / 5.0 / t;
    }

    inline static double
    bBsp(double l, double t) {
        return C1 / (l * l * l * l * l * (java::Math::exp(C2 / (t * l)) - 1.0));
    }

  public:
    int clock; // Incremented each change
    short flags; // What's been set
    short straightSamples[NUMBER_OF_SPECTRAL_SAMPLES]; // Spectral samples, min wl to max
    long spectralStraightSum; // Straight sum of spectral values
    float cx; // Chromaticity X value
    float cy; // Chromaticity Y value
    float eff; // Efficacy (lumens / watt)

    int setBlackBodyTemperature(double tk);
    void fixColorRepresentation(int fl);
    int setSpectrum(double wlMinimum, double wlMaximum, int ac, const char **av);

    void
    mixColors(
        double w1,
        ColorContext *c1,
        double w2,
        ColorContext *c2);
};

#endif
