#ifndef RVSD_TMBLN_RSHMR_TONE_MAP
#define RVSD_TMBLN_RSHMR_TONE_MAP

#include "tonemap/ToneMap.h"

class RevisedTumblinRushmeierToneMap: public ToneMap{ private:
    float g;
    float comp;
    float display;
    float lwaRTR;
    float ldaRTR;

    static float stevensGamma(float lum);

  public:
    RevisedTumblinRushmeierToneMap();
    ~RevisedTumblinRushmeierToneMap();

    void init(const ToneMappingContext &toneMapOptions);
    ColorRgb scaleForComputations(ColorRgb radiance) const;
    ColorRgb scaleForDisplay(ColorRgb radiance) const;
};

#endif
