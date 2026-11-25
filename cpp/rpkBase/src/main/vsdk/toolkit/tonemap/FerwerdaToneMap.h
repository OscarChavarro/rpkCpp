#ifndef __FERWERDA_TONE_MAP__
#define __FERWERDA_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

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

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions) final;

  public:
    FerwerdaToneMap();
    ~FerwerdaToneMap() final;

    ColorRgb scaleForComputations(const ColorRgb &radiance) const final;
    ColorRgb scaleForDisplay(const ColorRgb &radiance) const final;
};

#endif
