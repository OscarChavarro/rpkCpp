#ifndef __MGF_COLOR_CONTEXT__
#define __MGF_COLOR_CONTEXT__

#define C_CMINWL 380 // Minimum wavelength
#define C_CMAXWL 780 // Maximum wavelength
#define C_CWLI ((float)(C_CMAXWL-C_CMINWL) / (float)(NUMBER_OF_SPECTRAL_SAMPLES - 1))
#define C_CMAXV 10000 // Nominal maximum sample value
#define C_CLPWM (683.0/C_CMAXV) // Peak lumens/watt multiplier
#define C_CSSPEC 01 // Flag if spectrum is set
#define C_CDSPEC 02 // Flag if defined w/ spectrum
#define C_CSXY 04 // Flag if xy is set
#define C_CDXY 010 // Flag if defined w/ xy
#define C_CSEFF 020 // Flag if efficacy set

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

extern MgfColorContext *GLOBAL_mgf_currentColor;

extern void mgfContextFixColorRepresentation(MgfColorContext *clr, int fl);

#endif