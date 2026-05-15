#ifndef __WARD_TONE_MAP__
#define __WARD_TONE_MAP__

#include "tonemap/ToneMap.h"

class WardToneMap: public ToneMap{ private:
    float comp;
    float display;
    float lda;

  public:
    WardToneMap();
    ~WardToneMap();

    void init(const ToneMappingContext &toneMapOptions);
    ColorRgb scaleForComputations(ColorRgb radiance) const;
    ColorRgb scaleForDisplay(ColorRgb radiance) const;
};

#endif
