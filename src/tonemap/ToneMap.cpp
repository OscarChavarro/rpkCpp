#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "common/linealAlgebra/Numeric.h"
#include "tonemap/IdentityToneMap.h"

ToneMap::ToneMap() {
}

ToneMap::~ToneMap() {
}

static void
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

/**
Does most to convert radiance to display RGB color
1) radiance compression: from the high dynamic range in reality to
   the limited range of the computer screen.
2) colormodel conversion from the color model used for the computations to
   an RGB triplet for display on the screen
3) clipping of RGB values to the range [0,1].
*/
ColorRgb *
radianceToRgb(ColorRgb color, ColorRgb *rgb) {
    rescaleRadiance(color, &color);
    rgb->set(color.r, color.g, color.b);
    rgb->clip();
    return rgb;
}
