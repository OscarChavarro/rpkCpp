#include "java/lang/Math.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/logging/LoggerFormat.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/tonemap/LightnessToneMap.h"

LightnessToneMap::LightnessToneMap() {
}

LightnessToneMap::~LightnessToneMap() {
}

void
LightnessToneMap::initParameters(const ToneMappingContext &/*toneMapOptions*/) {
}

ColorRgb
LightnessToneMap::scaleForComputations(ColorRgb radiance) const {
    Logger::reportMessage("LightnessToneMap", Logger::WARNING, "ScaleForComputations", LoggerFormat::format("%s %d not yet implemented", __FILE__, __LINE__));
    return radiance;
}

ColorRgb
LightnessToneMap::scaleForDisplay(ColorRgb radiance) const {
    double max = Math::max(Math::max(radiance.getR(), radiance.getG()), radiance.getB());
    if ( max < 1e-32 ) {
        return radiance;
    }

    // Multiply by WHITE EFFICACY to convert W/m^2sr to nits
    // (reference luminance is also in nits)
    double scaleFactor = lightness(getColorSpace().getLuminousEfficacy() * max);
    if ( scaleFactor == 0.0 ) {
        return radiance;
    }

    radiance = ColorRgb(
        radiance.getR() * (scaleFactor / max),
        radiance.getG() * (scaleFactor / max),
        radiance.getB() * (scaleFactor / max));
    return radiance;
}

double
LightnessToneMap::lightness(double luminance) {
    if ( Statistics::instance().radiance.referenceLuminance == 0.0 ) {
        return 0.0;
    }

    double relativeLuminance = luminance / ((double)(Statistics::instance().radiance.referenceLuminance));
    if ( relativeLuminance > 0.008856 ) {
        return 1.16 * Math::pow(relativeLuminance, 0.33) - 0.16;
    } else {
        return 9.033 * relativeLuminance;
    }
}
