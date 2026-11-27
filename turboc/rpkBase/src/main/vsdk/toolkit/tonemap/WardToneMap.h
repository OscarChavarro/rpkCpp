#ifndef __WARD_TONE_MAP__
#define __WARD_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class WardToneMap: public ToneMap{ private:
    double comp;
    double display;
    double lda;

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions);

  public:
    WardToneMap();
    ~WardToneMap();

    ColorRgb scaleForComputations(ColorRgb radiance) const;
    ColorRgb scaleForDisplay(ColorRgb radiance) const;
};

#endif
