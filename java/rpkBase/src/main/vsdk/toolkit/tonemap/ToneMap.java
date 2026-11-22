package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.logging.Logger;

/**
Most of the functions have similar meaning as for a radiance or ray-tracing method
*/
public abstract class ToneMap {
    private static ToneMap activeToneMap;

    private static int gammaTableEntry(float x) {
        return (int)(x * (float)(1 << ToneMappingContext.GAMMA_TABLE_BITS));
    }

    private static void recomputeGammaTable(ToneMappingContext toneMapOptions, int index, double gamma) {
        if (gamma <= VSDK.EPSILON) {
            gamma = 1.0;
        }
        for (int i = 0; i <= (1 << ToneMappingContext.GAMMA_TABLE_BITS); i++) {
            toneMapOptions.gammaTab[index][i] =
                (float)Math.pow(
                    (double)i / (double)(1 << ToneMappingContext.GAMMA_TABLE_BITS),
                    1.0 / gamma);
        }
    }

    private static ColorRgbMutable toneMapScaleForDisplay(ColorRgbMutable radiance) {
        if (activeToneMap == null) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "ToneMap::toneMapScaleForDisplay", "No active tone map");
        }
        return activeToneMap.scaleForDisplay(radiance);
    }

    /**
Rescale real world radiance using properly set up tone mapping algorithm
*/
    private static ColorRgbMutable rescaleRadiance(ColorRgbMutable in, ColorRgbMutable out, ToneMappingContext toneMapOptions) {
        ColorRgbMutable scaledInput = new ColorRgbMutable(in.getR(), in.getG(), in.getB());
        scaledInput.scale(toneMapOptions.pow_bright_adjust);
        ColorRgbMutable scaled = ToneMap.toneMapScaleForDisplay(scaledInput);
        out.set(scaled.getR(), scaled.getG(), scaled.getB());
        return out;
    }

    public ToneMap() {
    }

    public abstract void init(ToneMappingContext toneMapOptions);

    /**
    Transforms luminance from cd/m^2 to lamberts. Luminance in lamberts
    is needed for example by algorithms that are based on experiments of
    Stevens and Stevens (original Tumblin-Rushmeier tone operator). The
    transformation rule comes from Glassner's book, table 13.3, seems to
    be OK.
    */
    protected static float tmoCandelaLambert(float a) {
        return a * (float)Math.PI * 1e-4f;
    }

    /**
    Transforms luminance from lamberts to cd/m^2 to lamberts.
    */
    protected static float tmoLambertCandela(float a) {
        return a / ((float)Math.PI * 1e-4f);
    }

    /**
    Knowing the display luminance "dl" this function determines the
    correct scaling value that transforms display luminance back into
    the real world luminance.
    */
    public abstract ColorRgbMutable scaleForComputations(ColorRgbMutable radiance);

    /**
    Full tone mapping to display values. Transforms real world luminance of
    colour specified by "radiance" into corresponding display input
    values. The result has to be clipped to <0,1> afterwards.
    */
    public abstract ColorRgbMutable scaleForDisplay(ColorRgbMutable radiance);

    public static void setActiveToneMap(ToneMap toneMap) {
        activeToneMap = toneMap;
    }

    public static void toneMappingGammaCorrection(ColorRgbMutable rgb, ToneMappingContext toneMapOptions) {
        rgb.r = toneMapOptions.gammaTab[0][gammaTableEntry((float)rgb.getR())];
        rgb.g = toneMapOptions.gammaTab[1][gammaTableEntry((float)rgb.getG())];
        rgb.b = toneMapOptions.gammaTab[2][gammaTableEntry((float)rgb.getB())];
    }

    /**
Recomputes gamma tables for the given gamma values for red, green and blue
*/
    public static void recomputeGammaTables(ToneMappingContext toneMapOptions, ColorRgbMutable gamma) {
        ToneMap.recomputeGammaTable(toneMapOptions, 0, gamma.getR());
        ToneMap.recomputeGammaTable(toneMapOptions, 1, gamma.getG());
        ToneMap.recomputeGammaTable(toneMapOptions, 2, gamma.getB());
    }

    /**
Does most to convert radiance to display RGB color
1) radiance compression: from the high dynamic range in reality to
   the limited range of the computer screen.
2) colormodel conversion from the color model used for the computations to
   an RGB triplet for display on the screen
3) clipping of RGB values to the range [0,1].
*/
    public static ColorRgbMutable radianceToRgb(ColorRgbMutable color, ColorRgbMutable rgb, ToneMappingContext toneMapOptions) {
        ColorRgbMutable rescaled = new ColorRgbMutable();
        ToneMap.rescaleRadiance(color, rescaled, toneMapOptions);
        rgb.set(rescaled.getR(), rescaled.getG(), rescaled.getB());
        rgb.clip();
        return rgb;
    }
}
