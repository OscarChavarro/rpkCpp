/**
Lightness tone map
*/

#include "common/error.h"
#include "material/cie.h"
#include "material/statistics.h"
#include "IMAGE/tonemap/lightness.h"

static void Defaults() {
}

static void Init() {
}

static void Terminate() {
}

static float Lightness(float luminance) {
    float relative_luminance;

    if ( GLOBAL_statistics_referenceLuminance == 0.0 ) {
        return 0.0f;
    }

    relative_luminance = luminance / GLOBAL_statistics_referenceLuminance;
    if ( relative_luminance > 0.008856 ) {
        return (float)(1.16f * (float)pow(relative_luminance, 0.33f) - 0.16f);
    } else {
        return 9.033f * relative_luminance;
    }
}

static COLOR ScaleForComputations(COLOR radiance) {
    logFatal(-1, "ScaleForComputations", "%s %d not yet implemented", __FILE__, __LINE__);
    return radiance;
}

static COLOR ScaleForDisplay(COLOR radiance) {
    float max, scale_factor;

    max = colorMaximumComponent(radiance);
    if ( max < 1e-32 ) {
        return radiance;
    }

    /* multiply by WHTEFFICACY to convert W/m^2sr to nits
     * (reference luuminance is also in nits) */
    scale_factor = Lightness(WHITE_EFFICACY * max);
    if ( scale_factor == 0. ) {
        return radiance;
    }

    colorScale((scale_factor / max), radiance, radiance);
    return radiance;
}

static float ReverseScaleForComputations(float dl) {
    logFatal(-1, "ReverseScaleForComputations", "%s %d not yet implemented", __FILE__, __LINE__);
    return -1.0;
}

TONEMAP TM_Lightness = {
        "Lightness Mapping", "Lightness", "tmoLightnessButton", 3,
        Defaults,
        (void (*)(int *, char **)) nullptr,
        (void (*)(FILE *)) nullptr,
        Init,
        Terminate,
        ScaleForComputations,
        ScaleForDisplay,
        ReverseScaleForComputations,
        (void (*)(void *)) nullptr,
        (void (*)(void *)) nullptr,
        (void (*)(void)) nullptr,
        (void (*)(void)) nullptr
};
