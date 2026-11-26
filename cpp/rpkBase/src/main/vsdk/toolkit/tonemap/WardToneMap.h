#ifndef __WARD_TONE_MAP__
#define __WARD_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class WardToneMap final : public ToneMap {
  private:
    double comp;
    double display;
    double lda;

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions) final;

  public:
    WardToneMap();
    ~WardToneMap() final;

    ColorRgb scaleForComputations(const ColorRgb &radiance) const final;
    ColorRgb scaleForDisplay(const ColorRgb &radiance) const final;
};

#endif
