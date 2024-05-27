#include "tonemap/IdentityToneMap.h"

IdentityToneMap::IdentityToneMap() {
}

IdentityToneMap::~IdentityToneMap() {
}

void
IdentityToneMap::defaults() {
}

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

OldToneMap GLOBAL_toneMap_dummy = {
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
