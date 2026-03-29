#ifndef __COLOR_CONTEXT__
#define __COLOR_CONTEXT__

#include "java/lang/Math.h"

enum {
COLOR_MINIMUM_WAVE_LENGTH = 380,
COLOR_MAXIMUM_WAVE_LENGTH = 780
};

constexpr int NUMBER_OF_SPECTRAL_SAMPLES = 41; // Number of spectral samples

constexpr int COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE = 10000;

enum {
COLOR_SPECTRUM_IS_SET_FLAG = 01,
COLOR_DEFINED_WITH_SPECTRUM_FLAG = 02,
COLOR_XY_IS_SET_FLAG = 04,
COLOR_DEFINED_WITH_XY_FLAG = 010,
COLOR_EFFICACY_FLAG = 020
};

// W-m^2
constexpr double C1 = 3.741832e-16;

// m-K
constexpr double C2 = 1.4388e-2;

class ColorContext {
  private:
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

extern const ColorContext DEFAULT_COLOR_CONTEXT;

#endif
