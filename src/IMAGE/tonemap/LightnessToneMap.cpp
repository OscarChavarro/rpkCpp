/**
Lightness tone map
*/

#include "common/error.h"
#include "common/cie.h"
#include "common/Statistics.h"
#include "IMAGE/tonemap/LightnessToneMap.h"

static void lightnessDefaults() {
}

static void lightnessInit() {
}

static void lightnessTerminate() {
}

static float Lightness(float luminance) {
    float relative_luminance;

    if ( GLOBAL_statistics.referenceLuminance == 0.0 ) {
        return 0.0f;
    }

    relative_luminance = luminance / (float)GLOBAL_statistics.referenceLuminance;
    if ( relative_luminance > 0.008856 ) {
        return 1.16f * java::Math::pow(relative_luminance, 0.33f) - 0.16f;
    } else {
        return 9.033f * relative_luminance;
    }
}

static ColorRgb lightnessScaleForComputations(ColorRgb radiance) {
    logWarning("ScaleForComputations", "%s %d not yet implemented", __FILE__, __LINE__);
    return radiance;
}

static ColorRgb
lightnessScaleForDisplay(ColorRgb radiance) {
    float max;
    float scaleFactor;

    max = radiance.maximumComponent();
    if ( max < 1e-32 ) {
        return radiance;
    }

    // Multiply by WHITE EFFICACY to convert W/m^2sr to nits
    // (reference luminance is also in nits)
    scaleFactor = Lightness(WHITE_EFFICACY * max);
    if ( scaleFactor == 0.0 ) {
        return radiance;
    }

    radiance.scale(scaleFactor / max);
    return radiance;
}

ToneMap GLOBAL_toneMap_lightness = {
    "Lightness Mapping",
    "Lightness",
    3,
    lightnessDefaults,
    nullptr,
    lightnessInit,
    lightnessTerminate,
    lightnessScaleForComputations,
    lightnessScaleForDisplay
};
