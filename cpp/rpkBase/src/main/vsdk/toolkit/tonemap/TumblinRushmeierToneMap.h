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

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions) final;

  public:
    TumblinRushmeierToneMap();
    ~TumblinRushmeierToneMap() final;

    ColorRgb scaleForComputations(const ColorRgb &radiance) const final;
    ColorRgb scaleForDisplay(const ColorRgb &radiance) const final;
};

#endif
