#ifndef __FERWERDA_TONE_MAP__
#define __FERWERDA_TONE_MAP__

#include "tonemap/ToneMap.h"

class FerwerdaToneMap final : public ToneMap {
  private:
    static float photopicOperator(float logLa);
    static float scotopicOperator(float logLa);
    static float mesopicScaleFactor(float logLwa);

  public:
    FerwerdaToneMap();
    ~FerwerdaToneMap() final;

    void init() final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
};

extern OldToneMap GLOBAL_toneMap_ferwerda;

#endif
