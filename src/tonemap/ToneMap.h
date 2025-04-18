#ifndef __TONE_MAP__
#define __TONE_MAP__

#include "java/util/ArrayList.h"
#include "tonemap/ToneMappingContext.h"

/**
Most of the functions have similar meaning as for a radiance or ray-tracing method
*/
class ToneMap {
  public:
    ToneMap();
    virtual ~ToneMap();
    virtual void init() = 0;

    /**
    Knowing the display luminance "dl" this function determines the
    correct scaling value that transforms display luminance back into
    the real world luminance.
    */
    virtual ColorRgb scaleForComputations(ColorRgb radiance) const = 0;

    /**
    Full tone mapping to display values. Transforms real world luminance of
    colour specified by "radiance" into corresponding display input
    values. The result has to be clipped to <0,1> afterwards.
    */
    virtual ColorRgb scaleForDisplay(ColorRgb radiance) const = 0;
};

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
    return GLOBAL_toneMap_options.selectedToneMap->scaleForDisplay(radiance);
}

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

extern void recomputeGammaTables(ColorRgb gamma);
extern ColorRgb *radianceToRgb(ColorRgb color, ColorRgb *rgb);

#endif
