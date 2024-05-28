#ifndef __TONE_MAP_LIGHTNESS__
#define __TONE_MAP_LIGHTNESS__

#include "tonemap/ToneMap.h"

class LightnessToneMap final : public ToneMap {
  private:
    static float lightness(float luminance);

  public:
    LightnessToneMap();
    ~LightnessToneMap() final;

    void init() final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
    ColorRgb scaleForDisplay(ColorRgb radiance) const final;
};

#endif
