#ifndef __TUMBLIN_RUSHMEIER_TONE_MAP__
#define __TUMBLIN_RUSHMEIER_TONE_MAP__

#include "tonemap/ToneMap.h"

class TumblinRushmeierToneMap final : public ToneMap {
  public:
    TumblinRushmeierToneMap();
    ~TumblinRushmeierToneMap() final;

    void init() final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
    ColorRgb scaleForDisplay(ColorRgb radiance) const final;
};

#endif
