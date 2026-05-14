#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"

ToneMap *ToneMap::activeToneMap = nullptr;

ToneMap::ToneMap() {
}

ToneMap::~ToneMap() {
}

int
ToneMap::gammaTableEntry(float x) {
    return static_cast<int>(x * static_cast<float>(1 << ToneMappingContext::GAMMA_TABLE_BITS));
}

void
ToneMap::toneMappingGammaCorrection(ColorRgb &rgb, const ToneMappingContext &toneMapOptions) {
    rgb.setR(toneMapOptions.gammaTab[0][gammaTableEntry(static_cast<float>(rgb.getR()))]);
    rgb.setG(toneMapOptions.gammaTab[1][gammaTableEntry(static_cast<float>(rgb.getG()))]);
    rgb.setB(toneMapOptions.gammaTab[2][gammaTableEntry(static_cast<float>(rgb.getB()))]);
}

ColorRgb
ToneMap::toneMapScaleForDisplay(const ColorRgb &radiance) {
    if ( activeToneMap == nullptr ) {
        Logger::fatal(-1, "ToneMap::toneMapScaleForDisplay", "No active tone map");
    }
    return activeToneMap->scaleForDisplay(radiance);
}

float
ToneMap::tmoCandelaLambert(float a) {
    return a * static_cast<float>(java::Math::PI) * 1e-4F;
}

float
ToneMap::tmoLambertCandela(float a) {
    return a / (static_cast<float>(java::Math::PI) * 1e-4F);
}

void
ToneMap::recomputeGammaTable(ToneMappingContext &toneMapOptions, int index, double gamma) {
    if ( gamma <= Numeric::EPSILON ) {
        gamma = 1.0;
    }
    for ( int i = 0; i <= (1 << ToneMappingContext::GAMMA_TABLE_BITS); i++ ) {
        toneMapOptions.gammaTab[index][i] =
            static_cast<float>(java::Math::pow(static_cast<double>(i) / static_cast<double>(1 << ToneMappingContext::GAMMA_TABLE_BITS),
            1.0 / gamma));
    }
}

/**
Recomputes gamma tables for the given gamma values for red, green and blue
*/
void
ToneMap::recomputeGammaTables(ToneMappingContext &toneMapOptions, ColorRgb gamma) {
    ToneMap::recomputeGammaTable(toneMapOptions, 0, gamma.getR());
    ToneMap::recomputeGammaTable(toneMapOptions, 1, gamma.getG());
    ToneMap::recomputeGammaTable(toneMapOptions, 2, gamma.getB());
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
    rgb->set(color.getR(), color.getG(), color.getB());
    rgb->clip();
    return rgb;
}
