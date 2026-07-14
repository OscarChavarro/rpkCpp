#include "vsdk/toolkit/tonemap/IdentityToneMap.h"

IdentityToneMap::IdentityToneMap() {
}

IdentityToneMap::~IdentityToneMap() {
}

void
IdentityToneMap::init(const ToneMappingContext &/*toneMapOptions*/) {
}

ColorRgbMutable
IdentityToneMap::scaleForComputations(ColorRgbMutable radiance) const {
    return radiance;
}

ColorRgbMutable
IdentityToneMap::scaleForDisplay(ColorRgbMutable radiance) const {
    return radiance;
}
