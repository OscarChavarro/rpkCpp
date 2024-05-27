#include "tonemap/IdentityToneMap.h"

static void identityDefaults() {
}

static void identityInit() {
}

static void identityTerminate() {
}

static ColorRgb identityScaleForComputations(ColorRgb radiance) {
    return radiance;
}

static ColorRgb identityScaleForDisplay(ColorRgb radiance) {
    return radiance;
}

ToneMap GLOBAL_toneMap_dummy = {
    "Dummy",
    "Dummy",
    3,
    identityDefaults,
    nullptr,
    identityInit,
    identityTerminate,
    identityScaleForComputations,
    identityScaleForDisplay
};
