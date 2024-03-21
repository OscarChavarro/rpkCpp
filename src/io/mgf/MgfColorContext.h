#ifndef __MGF_COLOR_CONTEXT__
#define __MGF_COLOR_CONTEXT__

#define DEFAULT_COLOR_CONTEXT { 1, C_CDXY | C_CSXY | C_CSSPEC | C_CSEFF, \
    { \
        C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, \
        C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, \
        C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, \
        C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, \
        C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, \
        C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, \
        C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV, C_CMAXV \
    }, \
    (long)NUMBER_OF_SPECTRAL_SAMPLES * C_CMAXV, 1.0 / 3.0, 1.0 / 3.0, 178.006 }

#define NUMBER_OF_SPECTRAL_SAMPLES 41 // Number of spectral samples

class MgfColorContext {
public:
    int clock; // Incremented each change
    short flags; // What's been set
    short straightSamples[NUMBER_OF_SPECTRAL_SAMPLES]; // Spectral samples, min wl to max
    long spectralStraightSum; // Straight sum of spectral values
    float cx; // Chromaticity X value
    float cy; // Chromaticity Y value
    float eff; // efficacy (lumens / watt)
};

#endif