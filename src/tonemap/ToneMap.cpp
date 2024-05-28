#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "common/linealAlgebra/Numeric.h"
#include "tonemap/IdentityToneMap.h"

ToneMap::ToneMap() {
}

ToneMap::~ToneMap() {
}

/**
Makes map the current tone mapping operator + initialises
*/
void
setToneMap(ToneMap *toneMap) {
    GLOBAL_toneMap_options.selectedToneMap = toneMap;
    toneMap->init();
}

void
recomputeGammaTable(int index, double gamma) {
    if ( gamma <= Numeric::EPSILON ) {
        gamma = 1.0;
    }
    for ( int i = 0; i <= (1 << GAMMA_TABLE_BITS); i++ ) {
        GLOBAL_toneMap_options.gammaTab[index][i] =
            (float)java::Math::pow((double) i / (double) (1 << GAMMA_TABLE_BITS), 1.0 / gamma);
    }
}

/**
Recomputes gamma tables for the given gamma values for red, green and blue
*/
void
recomputeGammaTables(ColorRgb gamma) {
    recomputeGammaTable(0, gamma.r);
    recomputeGammaTable(1, gamma.g);
    recomputeGammaTable(2, gamma.b);
}

/**
Rescale real world radiance using properly set up tone mapping algorithm
*/
ColorRgb *
rescaleRadiance(ColorRgb in, ColorRgb *out) {
    in.scale(GLOBAL_toneMap_options.pow_bright_adjust);
    *out = toneMapScaleForDisplay(in);
    return out;
}

ColorRgb *
radianceToRgb(ColorRgb color, ColorRgb *rgb) {
    rescaleRadiance(color, &color);
    rgb->set(color.r, color.g, color.b);
    rgb->clip();
    return rgb;
}
