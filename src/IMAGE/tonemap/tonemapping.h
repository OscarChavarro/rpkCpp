#ifndef __TONE_MAPPING__
#define __TONE_MAPPING__

#include "java/util/ArrayList.h"
#include "IMAGE/adaptation.h"

/**
Most of the functions have similar meaning as for a radiance or ray-tracing method
*/
class ToneMap {
  public:
    const char *name; // Full name
    const char *shortName; // Short name usable as option argument
    int abbrev; // Minimal abbreviation of short name in option argument

    void (*Defaults)(); // Sets defaults
    void (*ParseOptions)(int *argc, char **argv); // Optional
    void (*Init)(); // Initialises
    void (*Terminate)(); // Terminates

    /**
    Knowing the display luminance "dl" this function determines the
    correct scaling value that transforms display luminance back into
    the real world luminance.
    */
    ColorRgb (*scaleForComputations)(ColorRgb radiance);

    /**
    Full tone mapping to display values. Transforms real world luminance of
    colour specified by "radiance" into corresponding display input
    values. The result has to be clipped to <0,1> afterwards.
    */
    ColorRgb (*scaleForDisplay)(ColorRgb radiance);
};

// Available tone mapping operators (nullptr terminated array)
extern ToneMap *GLOBAL_toneMap_availableToneMaps[];

extern void setToneMap(ToneMap *map);

// Gamma correction table
#define GAMMA_TABLE_BITS 12
#define GAMMA_TABLE_SIZE ((1 << GAMMA_TABLE_BITS) + 1)

// Recomputes gamma tables for the given gamma values for red, green and blue
extern void recomputeGammaTables(ColorRgb gamma);

class ToneMappingContext {
  public:
    // Fixed radiance rescaling before tone mapping
    float brightness_adjust; // Brightness adjustment factor
    float pow_bright_adjust; // pow(2, brightness_adjust)

    // Variable / non-linear radiance rescaling
    ToneMap *toneMap; // Current tone mapping operator
    TMA_METHOD staticAdaptationMethod;
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

extern void toneMapDefaults();
extern void toneMapParseOptions(int *argc, char **argv);
extern void initToneMapping(java::ArrayList<Patch *> *scenePatches);

inline int
gammaTableEntry(float x) {
    return (int)(x * (float)(1 << GAMMA_TABLE_BITS));
}

inline void
toneMappingGammaCorrection(ColorRgb &rgb) {
    rgb.r = GLOBAL_toneMap_options.gammaTab[0][gammaTableEntry(rgb.r)];
    rgb.g = GLOBAL_toneMap_options.gammaTab[1][gammaTableEntry(rgb.g)];
    rgb.b = GLOBAL_toneMap_options.gammaTab[2][gammaTableEntry(rgb.b)];
}

inline ColorRgb
toneMapScaleForDisplay(const ColorRgb &radiance) {
    return GLOBAL_toneMap_options.toneMap->scaleForDisplay(radiance);
}

/**
Does most to convert radiance to display RGB color
1) radiance compression: from the high dynamic range in reality to
   the limited range of the computer screen.
2) colormodel conversion from the color model used for the computations to
   an RGB triplet for display on the screen
3) clipping of RGB values to the range [0,1].
*/
extern ColorRgb *radianceToRgb(ColorRgb color, ColorRgb *rgb);

/**
Transforms luminance from cd/m^2 to lamberts. Luminance in lamberts
is needed for example by algorithms that are based on experiments of
Stevens and Stevens (original Tumblin-Rushmeier tone operator). The
transformation rule comes from Glassner's book, table 13.3, seems to
be OK.
*/
inline float
tmoCandelaLambert(float a) {
    return a * (float)M_PI * 1e-4f;
}

/**
Transforms luminance from lamberts to cd/m^2 to lamberts.
*/
inline float
tmoLambertCandela(float a) {
    return a / ((float)M_PI * 1e-4f);
}

#endif
