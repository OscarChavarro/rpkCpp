#ifndef __REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__
#define __REVISED_TUMBLIN_RUSHMEIER_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class RevisedTumblinRushmeierToneMap final : public ToneMap {
  private:
    double g;
    double comp;
    double display;
    double lwaRTR;
    double ldaRTR;

    static double stevensGamma(double lum);

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions) final;

  public:
    RevisedTumblinRushmeierToneMap();
    ~RevisedTumblinRushmeierToneMap() final;

    ColorRgb scaleForComputations(const ColorRgb &radiance) const final;
    ColorRgb scaleForDisplay(const ColorRgb &radiance) const final;
};

#endif
