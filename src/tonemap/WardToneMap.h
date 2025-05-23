#ifndef __WARD_TONE_MAP__
#define __WARD_TONE_MAP__

#include "tonemap/ToneMappingContext.h"

class WardToneMap final : public ToneMap {
  private:
    float comp;
    float display;
    float lda;

  public:
    WardToneMap();
    ~WardToneMap() final;

    void init() final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
    ColorRgb scaleForDisplay(ColorRgb radiance) const final;
};

#endif
