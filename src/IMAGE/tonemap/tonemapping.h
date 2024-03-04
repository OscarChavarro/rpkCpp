#ifndef _RPK_TONE_MAPPING_
#define _RPK_TONE_MAPPING_

#include "java/util/ArrayList.h"
#include "IMAGE/adaptation.h"

/**
Most of the functions have similar meaning as for a radiance or ray-tracing method
*/
class TONEMAP {
  public:
    const char *name; // Full name
    const char *shortName; // Short name usable as option argument
    int abbrev; // Minimal abbreviation of short name in option argument

    void (*Defaults)(); // Sets defaults
    void (*ParseOptions)(int *argc, char **argv); // Optional
    void (*Init)(); // Initialises
    void (*Terminate)(); // Terminates

    /**
    `TonemapReverseScaleForComputations'

    Knowing the display luminance "dl" this function determines the
    correct scaling value that transforms display luminance back into
    the real world luminance.
    */
    COLOR (*ScaleForComputations)(COLOR radiance);

    /**
    `toneMapScaleForDisplay'

    Full tone mapping to display values. Transforms real world luminance of
    colour specified by "radiance" into corresponding display input
    values. The result has to be clipped to <0,1> afterwards.
    */
    COLOR (*ScaleForDisplay)(COLOR radiance);
};

/* available tone mapping operators (nullptr terminated array) */
extern TONEMAP *GLOBAL_toneMap_availableToneMaps[];

/* iterates over all available tone maps */
#define ForAllAvailableToneMaps(map)  {{    \
  TONEMAP **mapp;                \
  for (mapp=GLOBAL_toneMap_availableToneMaps; *mapp; mapp++) { \
    TONEMAP *map = *mapp;

#ifndef EndForAll
#define EndForAll }}}
#endif

extern void setToneMap(TONEMAP *map);

/* gamma correction table */
#define GAMMATAB_BITS  12  /* 12-bit gamma table */
#define GAMMATAB_SIZE  ((1<<GAMMATAB_BITS)+1)

/* convert RGB color value between 0 and 1 to entry index in gamma table */
#define GAMMATAB_ENTRY(x)  (int)((x)*(float)(1<<GAMMATAB_BITS))

/* recomputes gamma tables for the given gamma values for 
 * red, green and blue */
extern void recomputeGammaTables(RGB gamma);

/* tone mapping global variables */
class TONEMAPPINGCONTEXT {
  public:
    /* fixed radiance rescaling before tone mapping */
    float brightness_adjust,    /* brightness adjustment factor */
    pow_bright_adjust;    /* pow(2, brightness_adjust) */

    /* variable/non-linear radiance rescaling */
    TONEMAP *ToneMap;        /* current tone mapping operator */
    TMA_METHOD statadapt;        /* static adaptation method */
    float lwa,                    /* jp: real world adaption luminance */
    ldm,                    /* jp: maximum display luminance */
    cmax;                    /* jp: maximum display contrast */

    /* conversion from radiance (COLOR type) to display RGB */
    float xr, yr, xg, yg, xb, yb,    /* monitor primary colors */
    xw, yw;            /* monitor white point */

    /* display RGB mapping (corrects display non-linear response) */
    RGB gamma;                  /* gamma factors for red, green, blue */
    float gammatab[3][GAMMATAB_SIZE]; /* gamma correction tables for red, green and blue */
};
extern TONEMAPPINGCONTEXT GLOBAL_toneMap_options;

extern void toneMapDefaults();
extern void parseToneMapOptions(int *argc, char **argv);
extern void initToneMapping(java::ArrayList<Patch *> *scenePatches);

inline void
toneMappingGammaCorrection(RGB &rgb) {
  (rgb).r = GLOBAL_toneMap_options.gammatab[0][GAMMATAB_ENTRY((rgb).r)];
  (rgb).g = GLOBAL_toneMap_options.gammatab[1][GAMMATAB_ENTRY((rgb).g)];
  (rgb).b = GLOBAL_toneMap_options.gammatab[2][GAMMATAB_ENTRY((rgb).b)];
}

/* shortcuts */
#define TonemapScaleForComputations(radiance)  (tmopts.ToneMap->ScaleForComputations(radiance))

inline COLOR
toneMapScaleForDisplay(COLOR &radiance) {
    return GLOBAL_toneMap_options.ToneMap->ScaleForDisplay(radiance);
}

/* Does most to convert radiance to display RGB color
 * 1) radiance compression: from the high dynamic range in reality to
 *    the limited range of the computer screen.
 * 2) colormodel conversion from the color model used for the computations to
 *    an RGB triplet for display on the screen
 * 3) clipping of RGB values to the range [0,1].
 * Gamma correction is performed in render.c. */
extern RGB *radianceToRgb(COLOR color, RGB *rgb);

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
