#include "java/lang/Math.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/color/Cie.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/tonemap/LightnessToneMap.h"

LightnessToneMap::LightnessToneMap() {
}

LightnessToneMap::~LightnessToneMap() {
}

void
LightnessToneMap::init(const ToneMappingContext &/*toneMapOptions*/) {
}

ColorRgb
LightnessToneMap::scaleForComputations(const ColorRgb &radiance) const {
    Logger::reportMessage("LightnessToneMap", Logger::WARNING, "ScaleForComputations", java::String::format("%s %d not yet implemented", __FILE__, __LINE__));
    return radiance;
}

ColorRgb
LightnessToneMap::scaleForDisplay(const ColorRgb &radiance) const {
    const double r = radiance.getR();
    const double g = radiance.getG();
    const double b = radiance.getB();
    const float max = r > g ? (r > b ? r : b) : (g > b ? g : b);
    if ( max < 1e-32 ) {
        return radiance;
    }

    // Multiply by WHITE EFFICACY to convert W/m^2sr to nits
    // (reference luminance is also in nits)
    const float scaleFactor = lightness(Cie::WHITE_EFFICACY * max);
    if ( scaleFactor == 0.0 ) {
        return radiance;
    }

    return radiance.multiply(scaleFactor / max);
}

float
LightnessToneMap::lightness(float luminance) {
    if ( Statistics::instance().radiance.referenceLuminance == 0.0 ) {
        return 0.0F;
    }

    const float relativeLuminance = luminance / static_cast<float>(Statistics::instance().radiance.referenceLuminance);
    if ( relativeLuminance > 0.008856 ) {
        return 1.16F * java::Math::pow(relativeLuminance, 0.33F) - 0.16F;
    } else {
        return 9.033F * relativeLuminance;
    }
}
