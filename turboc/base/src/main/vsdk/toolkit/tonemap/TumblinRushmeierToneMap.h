#ifndef __TUMBLIN_RUSHMEIER_TONE_MAP__
#define __TUMBLIN_RUSHMEIER_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class TumblinRushmeierToneMap: public ToneMap{ private:
    float invCMaximum;
    float lrwmComp;
    float lrwmDisplay;
    float lrwExponent;
    float lda;

  public:
    TumblinRushmeierToneMap();
    ~TumblinRushmeierToneMap();

    void init(const ToneMappingContext &toneMapOptions);
    ColorRgb scaleForComputations(ColorRgb radiance) const;
    ColorRgb scaleForDisplay(ColorRgb radiance) const;
};

#endif
