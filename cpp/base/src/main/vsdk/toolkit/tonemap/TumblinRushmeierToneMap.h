#ifndef TUMBLIN_RUSHMEIER_TONE_MAP__
#define TUMBLIN_RUSHMEIER_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class TumblinRushmeierToneMap final : public ToneMap {
  private:
    float invCMaximum;
    float lrwmComp;
    float lrwmDisplay;
    float lrwExponent;
    float lda;

  public:
    TumblinRushmeierToneMap();
    ~TumblinRushmeierToneMap() final;

    void init(const ToneMappingContext &toneMapOptions) final;
    ColorRgbMutable scaleForComputations(ColorRgbMutable radiance) const final;
    ColorRgbMutable scaleForDisplay(ColorRgbMutable radiance) const final;
};

#endif
