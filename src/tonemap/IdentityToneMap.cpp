#include "tonemap/IdentityToneMap.h"

IdentityToneMap::IdentityToneMap() {
}

IdentityToneMap::~IdentityToneMap() {
}

void
IdentityToneMap::init() {
}

static ColorRgb
identityScaleForComputations(ColorRgb radiance) {
    return radiance;
}

static ColorRgb
identityScaleForDisplay(ColorRgb radiance) {
    return radiance;
}

OldToneMap GLOBAL_toneMap_identity = {
    "Dummy",
    "Dummy",
    3,
    nullptr,
    identityScaleForComputations,
    identityScaleForDisplay
};
