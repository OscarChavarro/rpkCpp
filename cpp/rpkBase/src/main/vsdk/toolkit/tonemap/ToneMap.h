#ifndef TONE_MAP__
#define TONE_MAP__

#include "java/lang/Math.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/tonemap/ToneMappingContext.h"

/**
Most of the functions have similar meaning as for a radiance or ray-tracing method
*/
class ToneMap {
  private:
    static ToneMap *activeToneMap;
    Cie colorSpace;
    static int gammaTableEntry(float x);
    static void recomputeGammaTable(ToneMappingContext &toneMapOptions, int index, double gamma);
    static ColorRgb toneMapScaleForDisplay(const ColorRgb &radiance);

  protected:
    const Cie &getColorSpace() const { return colorSpace; }

    /**
    Computes the parameters of the concrete operator from the context
    */
    virtual void initParameters(const ToneMappingContext &toneMapOptions) = 0;

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
    void init(const ToneMappingContext &toneMapOptions);

    /**
    Knowing the display luminance "dl" this function determines the
    correct scaling value that transforms display luminance back into
    the real world luminance.
    */
    virtual ColorRgb scaleForComputations(const ColorRgb &radiance) const = 0;

    /**
    Full tone mapping to display values. Transforms real world luminance of
    colour specified by "radiance" into corresponding display input
    values. The result has to be clipped to <0,1> afterwards.
    */
    virtual ColorRgb scaleForDisplay(const ColorRgb &radiance) const = 0;

    static void setActiveToneMap(ToneMap *toneMap);
    static ColorRgb toneMappingGammaCorrection(const ColorRgb &rgb, const ToneMappingContext &toneMapOptions);
    static void recomputeGammaTables(ToneMappingContext &toneMapOptions, ColorRgb gamma);
    static ColorRgb radianceToRgb(const ColorRgb &color, const ToneMappingContext &toneMapOptions);
};

#endif
