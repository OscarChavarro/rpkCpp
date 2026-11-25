#ifndef __LIGHTNESS_TONE_MAP__
#define __LIGHTNESS_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class LightnessToneMap final : public ToneMap {
  private:
    static float lightness(float luminance);

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions) final;

  public:
    LightnessToneMap();
    ~LightnessToneMap() final;

    ColorRgb scaleForComputations(const ColorRgb &radiance) const final;
    ColorRgb scaleForDisplay(const ColorRgb &radiance) const final;
};

#endif
