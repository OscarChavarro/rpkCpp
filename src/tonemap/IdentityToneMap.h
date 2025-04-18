#ifndef __IDENTITY_TONE_MAP__
#define __IDENTITY_TONE_MAP__

#include "tonemap/ToneMap.h"

class IdentityToneMap final : public ToneMap {
  public:
    IdentityToneMap();
    ~IdentityToneMap() final;

    void init() final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
    ColorRgb scaleForDisplay(ColorRgb radiance) const final;
};

#endif
