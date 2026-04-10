#ifndef __TONE_MAP_LIGHTNESS__
#define __TONE_MAP_LIGHTNESS__

#include "tonemap/ToneMap.h"

class LightnessToneMap: public ToneMap{ private:
    static float lightness(float luminance);

  public:
    LightnessToneMap();
    ~LightnessToneMap();

    void init(const ToneMappingContext &toneMapOptions);
    ColorRgb scaleForComputations(ColorRgb radiance) const;
    ColorRgb scaleForDisplay(ColorRgb radiance) const;
};

#endif
