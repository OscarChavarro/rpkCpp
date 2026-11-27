#ifndef RVSD_TMBLN_RSHMR_TONE_MAP
#define RVSD_TMBLN_RSHMR_TONE_MAP

#include "vsdk/toolkit/tonemap/ToneMap.h"

class RevisedTumblinRushmeierToneMap: public ToneMap{ private:
    double g;
    double comp;
    double display;
    double lwaRTR;
    double ldaRTR;

    static double stevensGamma(double lum);

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions);

  public:
    RevisedTumblinRushmeierToneMap();
    ~RevisedTumblinRushmeierToneMap();

    ColorRgb scaleForComputations(ColorRgb radiance) const;
    ColorRgb scaleForDisplay(ColorRgb radiance) const;
};

#endif
