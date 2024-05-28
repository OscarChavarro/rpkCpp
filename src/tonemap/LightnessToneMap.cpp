#include "java/lang/Math.h"
#include "common/error.h"
#include "common/cie.h"
#include "common/Statistics.h"
#include "tonemap/LightnessToneMap.h"

LightnessToneMap::LightnessToneMap() {
}

LightnessToneMap::~LightnessToneMap() {
}

void
LightnessToneMap::init() {
}

ColorRgb
LightnessToneMap::scaleForComputations(ColorRgb radiance) const {
    logWarning("ScaleForComputations", "%s %d not yet implemented", __FILE__, __LINE__);
    return radiance;
}

ColorRgb
LightnessToneMap::scaleForDisplay(ColorRgb radiance) const {
    float max = radiance.maximumComponent();
    if ( max < 1e-32 ) {
        return radiance;
    }

    // Multiply by WHITE EFFICACY to convert W/m^2sr to nits
    // (reference luminance is also in nits)
    float scaleFactor = lightness(WHITE_EFFICACY * max);
    if ( scaleFactor == 0.0 ) {
        return radiance;
    }

    radiance.scale(scaleFactor / max);
    return radiance;
}

float
LightnessToneMap::lightness(float luminance) {
    if ( GLOBAL_statistics.referenceLuminance == 0.0 ) {
        return 0.0f;
    }

    float relativeLuminance = luminance / (float)GLOBAL_statistics.referenceLuminance;
    if ( relativeLuminance > 0.008856 ) {
        return 1.16f * java::Math::pow(relativeLuminance, 0.33f) - 0.16f;
    } else {
        return 9.033f * relativeLuminance;
    }
}
