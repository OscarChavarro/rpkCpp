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
LightnessToneMap::scaleForComputations(ColorRgb radiance) const {
    Logger::warning("ScaleForComputations", "%s %d not yet implemented", __FILE__, __LINE__);
    return radiance;
}

ColorRgb
LightnessToneMap::scaleForDisplay(ColorRgb radiance) const {
    const float max = radiance.maximumComponent();
    if ( max < 1e-32 ) {
        return radiance;
    }

    // Multiply by WHITE EFFICACY to convert W/m^2sr to nits
    // (reference luminance is also in nits)
    const float scaleFactor = lightness(Cie::WHITE_EFFICACY * max);
    if ( scaleFactor == 0.0 ) {
        return radiance;
    }

    radiance.scale(scaleFactor / max);
    return radiance;
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
