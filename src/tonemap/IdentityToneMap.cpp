#include "tonemap/IdentityToneMap.h"

IdentityToneMap::IdentityToneMap() {
}

IdentityToneMap::~IdentityToneMap() {
}

void
IdentityToneMap::init() {
}

ColorRgb
IdentityToneMap::scaleForComputations(ColorRgb radiance) const {
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
    identityScaleForDisplay
};
