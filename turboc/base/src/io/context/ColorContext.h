#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __COLOR_CONTEXT__
#define __COLOR_CONTEXT__

#include "java/lang/Math.h"

enum {
    COLOR_MINIMUM_WAVE_LENGTH = 380,
    COLOR_MAXIMUM_WAVE_LENGTH = 780
};

enum {
    COLOR_SPECTRUM_IS_SET_FLAG = 01,
    CLR_DFND_WITH_SPCTR_FLAG = 02,
    COLOR_XY_IS_SET_FLAG = 04,
    COLOR_DEFINED_WITH_XY_FLAG = 010,
    COLOR_EFFICACY_FLAG = 020
};

class ColorContext {
  public:
    enum{
        NUMBER_OF_SPECTRAL_SAMPLES = 41, // Number of spectral samples
        CLR_NOM_MAX_SMP_VAL = 10000
    };
    static const ColorContext DEFAULT_COLOR_CONTEXT;

  private:
    // W-m^2
    static const double C1;
    // m-K
    static const double C2;
    static ColorContext cie_xp;
    static ColorContext cie_yp;
    static ColorContext cie_zp;
    static ColorContext cie_xf;
    static ColorContext cie_yf;
    static ColorContext cie_zf;

    inline static float
    colorWaveLengthDeltaI() {
        return ((float)(COLOR_MAXIMUM_WAVE_LENGTH - COLOR_MINIMUM_WAVE_LENGTH)) /
               ((float)(NUMBER_OF_SPECTRAL_SAMPLES - 1));
    }

    inline static double
    colorPeakLumensPerWatt() {
        return 683.0 / CLR_NOM_MAX_SMP_VAL;
    }

    inline static double
    bBlm(double t) {
        return C2 / 5.0 / t;
    }

    inline static double
    bBsp(double l, double t) {
        return C1 / (l * l * l * l * l * (Math::exp(C2 / (t * l)) - 1.0));
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
