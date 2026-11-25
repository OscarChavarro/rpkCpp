#ifndef REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__
#define REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class RevisedTumblinRushmeierToneMap final : public ToneMap {
  private:
    float g;
    float comp;
    float display;
    float lwaRTR;
    float ldaRTR;

    static float stevensGamma(float lum);

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions) final;

  public:
    RevisedTumblinRushmeierToneMap();
    ~RevisedTumblinRushmeierToneMap() final;

    ColorRgb scaleForComputations(const ColorRgb &radiance) const final;
    ColorRgb scaleForDisplay(const ColorRgb &radiance) const final;
};

#endif
