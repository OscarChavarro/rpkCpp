#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "common/linealAlgebra/Numeric.h"
#include "tonemap/ToneMap.h"

ToneMap *ToneMap::activeToneMap = NULL;

ToneMap::ToneMap() {
}

ToneMap::~ToneMap() {
}

int
ToneMap::gammaTableEntry(float x) {
    return ((int)(x * ((float)(1 << ToneMappingContext::GAMMA_TABLE_BITS))));
}

void
ToneMap::toneMappingGammaCorrection(ColorRgb &rgb, const ToneMappingContext &toneMapOptions) {
    rgb.r = toneMapOptions.gammaTab[0][gammaTableEntry(rgb.r)];
    rgb.g = toneMapOptions.gammaTab[1][gammaTableEntry(rgb.g)];
    rgb.b = toneMapOptions.gammaTab[2][gammaTableEntry(rgb.b)];
}

ColorRgb
ToneMap::toneMapScaleForDisplay(const ColorRgb &radiance) {
    if ( activeToneMap == NULL ) {
        Error::fatal(-1, "ToneMap::toneMapScaleForDisplay", "No active tone map");
    }
    return activeToneMap->scaleForDisplay(radiance);
}

float
ToneMap::tmoCandelaLambert(float a) {
    return a * ((float)(PI)) * 1e-4f;
}

float
ToneMap::tmoLambertCandela(float a) {
    return a / (((float)(PI)) * 1e-4f);
}

void
ToneMap::recomputeGammaTable(ToneMappingContext &toneMapOptions, int index, double gamma) {
    if ( gamma <= Numeric::EPSILON ) {
        gamma = 1.0;
    }
    for ( int i = 0; i <= (1 << ToneMappingContext::GAMMA_TABLE_BITS); i++ ) {
        toneMapOptions.gammaTab[index][i] =
            ((float)(Math::pow(((double)(i)) / ((double)(1 << ToneMappingContext::GAMMA_TABLE_BITS)),
            1.0 / gamma)));
    }
}

/**
Recomputes gamma tables for the given gamma values for red, green and blue
*/
void
ToneMap::recomputeGammaTables(ToneMappingContext &toneMapOptions, ColorRgb gamma) {
    ToneMap::recomputeGammaTable(toneMapOptions, 0, gamma.r);
    ToneMap::recomputeGammaTable(toneMapOptions, 1, gamma.g);
    ToneMap::recomputeGammaTable(toneMapOptions, 2, gamma.b);
}

/**
Rescale real world radiance using properly set up tone mapping algorithm
*/
ColorRgb *
ToneMap::rescaleRadiance(ColorRgb in, ColorRgb *out, const ToneMappingContext &toneMapOptions) {
    in.scale(toneMapOptions.pow_bright_adjust);
    *out = ToneMap::toneMapScaleForDisplay(in);
    return out;
}

void
ToneMap::setActiveToneMap(ToneMap *toneMap) {
    activeToneMap = toneMap;
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
ToneMap::radianceToRgb(ColorRgb color, ColorRgb *rgb, const ToneMappingContext &toneMapOptions) {
    ToneMap::rescaleRadiance(color, &color, toneMapOptions);
    rgb->set(color.r, color.g, color.b);
    rgb->clip();
    return rgb;
}
