#include "common/logging/Logger.h"
#include "common/color/Cie.h"
#include "common/statistics/Statistics.h"
#include "tonemap/LightnessToneMap.h"

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
    float max = Math::max(Math::max(radiance.getR(), radiance.getG()), radiance.getB());
    if ( max < 1e-32 ) {
        return radiance;
    }

    // Multiply by WHITE EFFICACY to convert W/m^2sr to nits
    // (reference luminance is also in nits)
    float scaleFactor = lightness(Cie::WHITE_EFFICACY * max);
    if ( scaleFactor == 0.0 ) {
        return radiance;
    }

    radiance = ColorRgb(
        radiance.getR() * (scaleFactor / max),
        radiance.getG() * (scaleFactor / max),
        radiance.getB() * (scaleFactor / max));
    return radiance;
}

float
LightnessToneMap::lightness(float luminance) {
    if ( Statistics::instance().radiance.referenceLuminance == 0.0 ) {
        return 0.0f;
    }

    float relativeLuminance = luminance / ((float)(Statistics::instance().radiance.referenceLuminance));
    if ( relativeLuminance > 0.008856 ) {
        return 1.16f * Math::pow(relativeLuminance, 0.33f) - 0.16f;
    } else {
        return 9.033f * relativeLuminance;
    }
}
