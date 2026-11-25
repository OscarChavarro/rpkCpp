#ifndef WARD_TONE_MAP__
#define WARD_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class WardToneMap final : public ToneMap {
  private:
    float comp;
    float display;
    float lda;

  public:
    WardToneMap();
    ~WardToneMap() final;

    void init(const ToneMappingContext &toneMapOptions) final;
    ColorRgb scaleForComputations(const ColorRgb &radiance) const final;
    ColorRgb scaleForDisplay(const ColorRgb &radiance) const final;
};

#endif
