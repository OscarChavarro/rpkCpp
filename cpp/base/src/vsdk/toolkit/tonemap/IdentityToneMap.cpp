#include "vsdk/toolkit/tonemap/IdentityToneMap.h"

IdentityToneMap::IdentityToneMap() {
}

IdentityToneMap::~IdentityToneMap() {
}

void
IdentityToneMap::init(const ToneMappingContext &/*toneMapOptions*/) {
}

ColorRgb
IdentityToneMap::scaleForComputations(ColorRgb radiance) const {
    return radiance;
}

ColorRgb
IdentityToneMap::scaleForDisplay(ColorRgb radiance) const {
    return radiance;
}
