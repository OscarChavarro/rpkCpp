#ifndef WARD_TONE_MAP__
#define WARD_TONE_MAP__

#include "tonemap/ToneMap.h"

class WardToneMap final : public ToneMap {
  private:
    float comp;
    float display;
    float lda;

  public:
    WardToneMap();
    ~WardToneMap() final;

    void init(const ToneMappingContext &toneMapOptions) final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
    ColorRgb scaleForDisplay(ColorRgb radiance) const final;
};

#endif
