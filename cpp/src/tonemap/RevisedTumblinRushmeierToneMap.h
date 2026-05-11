#ifndef REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__
#define REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__

#include "tonemap/ToneMap.h"

class RevisedTumblinRushmeierToneMap final : public ToneMap {
  private:
    float g;
    float comp;
    float display;
    float lwaRTR;
    float ldaRTR;

    static float stevensGamma(float lum);

  public:
    RevisedTumblinRushmeierToneMap();
    ~RevisedTumblinRushmeierToneMap() final;

    void init(const ToneMappingContext &toneMapOptions) final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
    ColorRgb scaleForDisplay(ColorRgb radiance) const final;
};

#endif
