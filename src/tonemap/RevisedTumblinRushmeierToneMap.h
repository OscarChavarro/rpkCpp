#ifndef __REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__
#define __REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__

#include "tonemap/ToneMap.h"

class RevisedTumblinRushmeierToneMap final : public ToneMap {
  private:
    static float stevensGamma(float lum);

  public:
    RevisedTumblinRushmeierToneMap();
    ~RevisedTumblinRushmeierToneMap() final;

    void init() final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
};

extern OldToneMap GLOBAL_toneMap_revisedTumblinRushmeier;

#endif
