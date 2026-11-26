#ifndef __FERWERDA_TONE_MAP__
#define __FERWERDA_TONE_MAP__

#include "vsdk/toolkit/tonemap/ToneMap.h"

class FerwerdaToneMap final : public ToneMap {
  private:
    ColorRgb sf;
    double msf;
    double pmComp;
    double pmDisplay;
    double smComp;
    double smDisplay;
    double lda;

    static double photopicOperator(double logLa);
    static double scotopicOperator(double logLa);
    static double mesopicScaleFactor(double logLwa);

  protected:
    void initParameters(const ToneMappingContext &toneMapOptions) final;

  public:
    FerwerdaToneMap();
    ~FerwerdaToneMap() final;

    ColorRgb scaleForComputations(const ColorRgb &radiance) const final;
    ColorRgb scaleForDisplay(const ColorRgb &radiance) const final;
};

#endif
