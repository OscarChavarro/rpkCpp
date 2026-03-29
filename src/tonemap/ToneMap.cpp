#include "java/util/ArrayList.txx"
#include "common/linealAlgebra/Numeric.h"
#include "tonemap/ToneMap.h"

ToneMap::ToneMap() {
}

ToneMap::~ToneMap() {
}

int
ToneMap::gammaTableEntry(float x) {
    return static_cast<int>(x * static_cast<float>(1 << GAMMA_TABLE_BITS));
}

void
ToneMap::toneMappingGammaCorrection(ColorRgb &rgb) {
    rgb.r = GLOBAL_toneMap_options.gammaTab[0][gammaTableEntry(rgb.r)];
    rgb.g = GLOBAL_toneMap_options.gammaTab[1][gammaTableEntry(rgb.g)];
    rgb.b = GLOBAL_toneMap_options.gammaTab[2][gammaTableEntry(rgb.b)];
}

ColorRgb
ToneMap::toneMapScaleForDisplay(const ColorRgb &radiance) {
    return GLOBAL_toneMap_options.selectedToneMap->scaleForDisplay(radiance);
}

float
ToneMap::tmoCandelaLambert(float a) {
    return a * static_cast<float>(java::Math::PI) * 1e-4f;
}

float
ToneMap::tmoLambertCandela(float a) {
    return a / (static_cast<float>(java::Math::PI) * 1e-4f);
}

void
ToneMap::recomputeGammaTable(int index, double gamma) {
    if ( gamma <= Numeric::EPSILON ) {
        gamma = 1.0;
    }
    for ( int i = 0; i <= (1 << GAMMA_TABLE_BITS); i++ ) {
        GLOBAL_toneMap_options.gammaTab[index][i] =
            static_cast<float>(java::Math::pow(static_cast<double>(i) / static_cast<double>(1 << GAMMA_TABLE_BITS),
            1.0 / gamma));
    }
}

/**
Recomputes gamma tables for the given gamma values for red, green and blue
*/
void
ToneMap::recomputeGammaTables(ColorRgb gamma) {
    ToneMap::recomputeGammaTable(0, gamma.r);
    ToneMap::recomputeGammaTable(1, gamma.g);
    ToneMap::recomputeGammaTable(2, gamma.b);
}

/**
Rescale real world radiance using properly set up tone mapping algorithm
*/
ColorRgb *
ToneMap::rescaleRadiance(ColorRgb in, ColorRgb *out) {
    in.scale(GLOBAL_toneMap_options.pow_bright_adjust);
    *out = ToneMap::toneMapScaleForDisplay(in);
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
ToneMap::radianceToRgb(ColorRgb color, ColorRgb *rgb) {
    ToneMap::rescaleRadiance(color, &color);
    rgb->set(color.r, color.g, color.b);
    rgb->clip();
    return rgb;
}
