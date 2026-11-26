#ifndef __TUMBLIN_RUSHMEIER_TONE_MAP__
#define __TUMBLIN_RUSHMEIER_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class TumblinRushmeierToneMap final : public ToneMap {
  private:
    double invCMaximum;
    double lrwmComp;
    double lrwmDisplay;
    double lrwExponent;
    double lda;

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions) final;

  public:
    TumblinRushmeierToneMap();
    ~TumblinRushmeierToneMap() final;

    ColorRgb scaleForComputations(const ColorRgb &radiance) const final;
    ColorRgb scaleForDisplay(const ColorRgb &radiance) const final;
};

#endif
