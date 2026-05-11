#ifndef FERWERDA_TONE_MAP__
#define FERWERDA_TONE_MAP__

#include "tonemap/ToneMap.h"

class FerwerdaToneMap final : public ToneMap {
  private:
    ColorRgb sf;
    float msf;
    float pmComp;
    float pmDisplay;
    float smComp;
    float smDisplay;
    float lda;

    static float photopicOperator(float logLa);
    static float scotopicOperator(float logLa);
    static float mesopicScaleFactor(float logLwa);

  public:
    FerwerdaToneMap();
    ~FerwerdaToneMap() final;

    void init(const ToneMappingContext &toneMapOptions) final;
    ColorRgb scaleForComputations(ColorRgb radiance) const final;
    ColorRgb scaleForDisplay(ColorRgb radiance) const final;
};

#endif
