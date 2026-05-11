#ifndef IDENTITY_TONE_MAP__
#define IDENTITY_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class IdentityToneMap final : public ToneMap {
  public:
    IdentityToneMap();
    ~IdentityToneMap() final;

    void init(const ToneMappingContext &toneMapOptions) final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
    ColorRgb scaleForDisplay(ColorRgb radiance) const final;
};

#endif
