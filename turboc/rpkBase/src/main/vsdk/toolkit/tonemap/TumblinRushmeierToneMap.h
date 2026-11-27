#ifndef __TUMBLIN_RUSHMEIER_TONE_MAP__
#define __TUMBLIN_RUSHMEIER_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class TumblinRushmeierToneMap: public ToneMap{ private:
    double invCMaximum;
    double lrwmComp;
    double lrwmDisplay;
    double lrwExponent;
    double lda;

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions);

  public:
    TumblinRushmeierToneMap();
    ~TumblinRushmeierToneMap();

    ColorRgb scaleForComputations(ColorRgb radiance) const;
    ColorRgb scaleForDisplay(ColorRgb radiance) const;
};

#endif
