#ifndef __LIGHTNESS_TONE_MAP__
#define __LIGHTNESS_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class LightnessToneMap: public ToneMap{ private:
    static double lightness(double luminance);

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions);

  public:
    LightnessToneMap();
    ~LightnessToneMap();

    ColorRgb scaleForComputations(ColorRgb radiance) const;
    ColorRgb scaleForDisplay(ColorRgb radiance) const;
};

#endif
