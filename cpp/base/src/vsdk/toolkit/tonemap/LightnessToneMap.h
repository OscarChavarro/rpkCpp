#ifndef TONE_MAP_LIGHTNESS__
#define TONE_MAP_LIGHTNESS__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class LightnessToneMap final : public ToneMap {
  private:
    static float lightness(float luminance);

  public:
    LightnessToneMap();
    ~LightnessToneMap() final;

    void init(const ToneMappingContext &toneMapOptions) final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
    ColorRgb scaleForDisplay(ColorRgb radiance) const final;
};

#endif
