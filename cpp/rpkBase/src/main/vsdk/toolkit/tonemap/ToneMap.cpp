#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"

ToneMap *ToneMap::activeToneMap = nullptr;

ToneMap::ToneMap(): colorSpace() {
}

ToneMap::~ToneMap() {
}

/**
Takes the display color space of the context, then lets the concrete
operator compute its parameters
*/
void
ToneMap::init(const ToneMappingContext &toneMapOptions) {
    colorSpace = toneMapOptions.getColorSpace();
    initParameters(toneMapOptions);
}

int
ToneMap::gammaTableEntry(double x) {
    return static_cast<int>(x * static_cast<double>(1 << ToneMappingContext::GAMMA_TABLE_BITS));
}

ColorRgb
ToneMap::toneMappingGammaCorrection(const ColorRgb &rgb, const ToneMappingContext &toneMapOptions) {
    return ColorRgb(
        toneMapOptions.gammaTab[0][gammaTableEntry(static_cast<double>(rgb.getR()))],
        toneMapOptions.gammaTab[1][gammaTableEntry(static_cast<double>(rgb.getG()))],
        toneMapOptions.gammaTab[2][gammaTableEntry(static_cast<double>(rgb.getB()))]);
}

ColorRgb
ToneMap::toneMapScaleForDisplay(const ColorRgb &radiance) {
    if ( activeToneMap == nullptr ) {
        Logger::reportMessage("ToneMap", Logger::FATAL_ERROR, "ToneMap::toneMapScaleForDisplay", "No active tone map");
    }
    return activeToneMap->scaleForDisplay(radiance);
}

double
ToneMap::tmoCandelaLambert(double a) {
    return a * static_cast<double>(java::Math::PI) * 1e-4;
}

double
ToneMap::tmoLambertCandela(double a) {
    return a / (static_cast<double>(java::Math::PI) * 1e-4);
}

void
ToneMap::recomputeGammaTable(ToneMappingContext &toneMapOptions, int index, double gamma) {
    if ( gamma <= VSDK::EPSILON ) {
        gamma = 1.0;
    }
    for ( int i = 0; i <= (1 << ToneMappingContext::GAMMA_TABLE_BITS); i++ ) {
        toneMapOptions.gammaTab[index][i] =
            static_cast<double>(java::Math::pow(static_cast<double>(i) / static_cast<double>(1 << ToneMappingContext::GAMMA_TABLE_BITS),
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

void
ToneMap::setActiveToneMap(ToneMap *toneMap) {
    activeToneMap = toneMap;
}

ColorRgb
ToneMap::radianceToRgb(const ColorRgb &color, const ToneMappingContext &toneMapOptions) {
    const ColorRgb scaled = toneMapScaleForDisplay(color.multiply(toneMapOptions.pow_bright_adjust));
    const double r = scaled.getR();
    const double g = scaled.getG();
    const double b = scaled.getB();
    return ColorRgb(r < 0.0 ? 0.0 : (r > 1.0 ? 1.0 : r),
                    g < 0.0 ? 0.0 : (g > 1.0 ? 1.0 : g),
                    b < 0.0 ? 0.0 : (b > 1.0 ? 1.0 : b));
}
