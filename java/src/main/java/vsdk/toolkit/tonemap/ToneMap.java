package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.Numeric;

/**
Most of the functions have similar meaning as for a radiance or ray-tracing method
*/
public abstract class ToneMap {
    private static ToneMap activeToneMap;

    private static int gammaTableEntry(float x) {
        return (int)(x * (float)(1 << ToneMappingContext.GAMMA_TABLE_BITS));
    }

    private static void recomputeGammaTable(ToneMappingContext toneMapOptions, int index, double gamma) {
        if (gamma <= Numeric.EPSILON) {
            gamma = 1.0;
        }
        for (int i = 0; i <= (1 << ToneMappingContext.GAMMA_TABLE_BITS); i++) {
            toneMapOptions.gammaTab[index][i] =
                (float)Math.pow(
                    (double)i / (double)(1 << ToneMappingContext.GAMMA_TABLE_BITS),
                    1.0 / gamma);
        }
    }

    private static ColorRgb toneMapScaleForDisplay(ColorRgb radiance) {
        if (activeToneMap == null) {
            Error.fatal(-1, "ToneMap::toneMapScaleForDisplay", "No active tone map");
        }
        return activeToneMap.scaleForDisplay(radiance);
    }

    /**
Rescale real world radiance using properly set up tone mapping algorithm
*/
    private static ColorRgb rescaleRadiance(ColorRgb in, ColorRgb out, ToneMappingContext toneMapOptions) {
        ColorRgb scaledInput = new ColorRgb(in.r, in.g, in.b);
        scaledInput.scale(toneMapOptions.pow_bright_adjust);
        ColorRgb scaled = ToneMap.toneMapScaleForDisplay(scaledInput);
        out.set(scaled.r, scaled.g, scaled.b);
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
    public abstract ColorRgb scaleForComputations(ColorRgb radiance);

    /**
    Full tone mapping to display values. Transforms real world luminance of
    colour specified by "radiance" into corresponding display input
    values. The result has to be clipped to <0,1> afterwards.
    */
    public abstract ColorRgb scaleForDisplay(ColorRgb radiance);

    public static void setActiveToneMap(ToneMap toneMap) {
        activeToneMap = toneMap;
    }

    public static void toneMappingGammaCorrection(ColorRgb rgb, ToneMappingContext toneMapOptions) {
        rgb.r = toneMapOptions.gammaTab[0][gammaTableEntry(rgb.r)];
        rgb.g = toneMapOptions.gammaTab[1][gammaTableEntry(rgb.g)];
        rgb.b = toneMapOptions.gammaTab[2][gammaTableEntry(rgb.b)];
    }

    /**
Recomputes gamma tables for the given gamma values for red, green and blue
*/
    public static void recomputeGammaTables(ToneMappingContext toneMapOptions, ColorRgb gamma) {
        ToneMap.recomputeGammaTable(toneMapOptions, 0, gamma.r);
        ToneMap.recomputeGammaTable(toneMapOptions, 1, gamma.g);
        ToneMap.recomputeGammaTable(toneMapOptions, 2, gamma.b);
    }

    /**
Does most to convert radiance to display RGB color
1) radiance compression: from the high dynamic range in reality to
   the limited range of the computer screen.
2) colormodel conversion from the color model used for the computations to
   an RGB triplet for display on the screen
3) clipping of RGB values to the range [0,1].
*/
    public static ColorRgb radianceToRgb(ColorRgb color, ColorRgb rgb, ToneMappingContext toneMapOptions) {
        ColorRgb rescaled = new ColorRgb();
        ToneMap.rescaleRadiance(color, rescaled, toneMapOptions);
        rgb.set(rescaled.r, rescaled.g, rescaled.b);
        rgb.clip();
        return rgb;
    }
}
