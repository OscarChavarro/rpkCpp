#ifndef TONE_MAP__
#define TONE_MAP__

#include "vsdk/toolkit/java/lang/Math.h"
#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

/**
Most of the functions have similar meaning as for a radiance or ray-tracing method
*/
class ToneMap {
  private:
    static ToneMap *activeToneMap;
    static int gammaTableEntry(float x);
    static void recomputeGammaTable(ToneMappingContext &toneMapOptions, int index, double gamma);
    static ColorRgbMutable toneMapScaleForDisplay(const ColorRgbMutable &radiance);
    static ColorRgbMutable *rescaleRadiance(ColorRgbMutable in, ColorRgbMutable *out, const ToneMappingContext &toneMapOptions);

  protected:
    /**
    Transforms luminance from cd/m^2 to lamberts. Luminance in lamberts
    is needed for example by algorithms that are based on experiments of
    Stevens and Stevens (original Tumblin-Rushmeier tone operator). The
    transformation rule comes from Glassner's book, table 13.3, seems to
    be OK.
    */
    static float tmoCandelaLambert(float a);

    /**
    Transforms luminance from lamberts to cd/m^2 to lamberts.
    */
    static float tmoLambertCandela(float a);

  public:
    ToneMap();
    virtual ~ToneMap();
    virtual void init(const ToneMappingContext &toneMapOptions) = 0;

    /**
    Knowing the display luminance "dl" this function determines the
    correct scaling value that transforms display luminance back into
    the real world luminance.
    */
    virtual ColorRgbMutable scaleForComputations(ColorRgbMutable radiance) const = 0;

    /**
    Full tone mapping to display values. Transforms real world luminance of
    colour specified by "radiance" into corresponding display input
    values. The result has to be clipped to <0,1> afterwards.
    */
    virtual ColorRgbMutable scaleForDisplay(ColorRgbMutable radiance) const = 0;

    static void setActiveToneMap(ToneMap *toneMap);
    static void toneMappingGammaCorrection(ColorRgbMutable &rgb, const ToneMappingContext &toneMapOptions);
    static void recomputeGammaTables(ToneMappingContext &toneMapOptions, ColorRgbMutable gamma);
    static ColorRgbMutable *radianceToRgb(ColorRgbMutable color, ColorRgbMutable *rgb, const ToneMappingContext &toneMapOptions);
};

#endif
