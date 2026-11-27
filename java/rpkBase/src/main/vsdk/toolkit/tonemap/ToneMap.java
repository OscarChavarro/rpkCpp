package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;

/**
Most of the functions have similar meaning as for a radiance or ray-tracing method
*/
public abstract class ToneMap {
    private static ToneMap activeToneMap;
    private Cie colorSpace;

    private static int gammaTableEntry(double x) {
        return (int)(x * (double)(1 << ToneMappingContext.GAMMA_TABLE_BITS));
    }

    private static void recomputeGammaTable(ToneMappingContext toneMapOptions, int index, double gamma) {
        if (gamma <= VSDK.EPSILON) {
            gamma = 1.0;
        }
        for (int i = 0; i <= (1 << ToneMappingContext.GAMMA_TABLE_BITS); i++) {
            toneMapOptions.gammaTab[index][i] =
                (double)Math.pow(
                    (double)i / (double)(1 << ToneMappingContext.GAMMA_TABLE_BITS),
                    1.0 / gamma);
        }
    }

    private static ColorRgb toneMapScaleForDisplay(ColorRgb radiance) {
        if (activeToneMap == null) {
            Logger.reportMessage(null, Logger.FATAL_ERROR, "ToneMap::toneMapScaleForDisplay", "No active tone map");
        }
        return activeToneMap.scaleForDisplay(radiance);
    }

    public ToneMap() {
        colorSpace = new Cie();
    }

    protected Cie getColorSpace() {
        return colorSpace;
    }

    /**
    Computes the parameters of the concrete operator from the context
    */
    protected abstract void initParameters(ToneMappingContext toneMapOptions);

    /**
    Takes the display color space of the context, then lets the concrete
    operator compute its parameters
    */
    public void init(ToneMappingContext toneMapOptions) {
        colorSpace = toneMapOptions.getColorSpace();
        initParameters(toneMapOptions);
    }

    /**
    Transforms luminance from cd/m^2 to lamberts. Luminance in lamberts
    is needed for example by algorithms that are based on experiments of
    Stevens and Stevens (original Tumblin-Rushmeier tone operator). The
    transformation rule comes from Glassner's book, table 13.3, seems to
    be OK.
    */
    protected static double tmoCandelaLambert(double a) {
        return a * (double)Math.PI * 1e-4;
    }

    /**
    Transforms luminance from lamberts to cd/m^2 to lamberts.
    */
    protected static double tmoLambertCandela(double a) {
        return a / ((double)Math.PI * 1e-4);
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

    public static ColorRgb toneMappingGammaCorrection(ColorRgb rgb, ToneMappingContext toneMapOptions) {
        return new ColorRgb(
            toneMapOptions.gammaTab[0][gammaTableEntry((double)rgb.getR())],
            toneMapOptions.gammaTab[1][gammaTableEntry((double)rgb.getG())],
            toneMapOptions.gammaTab[2][gammaTableEntry((double)rgb.getB())]);
    }

    /**
Recomputes gamma tables for the given gamma values for red, green and blue
*/
    public static void recomputeGammaTables(ToneMappingContext toneMapOptions, ColorRgb gamma) {
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
    public static ColorRgb radianceToRgb(ColorRgb color, ToneMappingContext toneMapOptions) {
        final ColorRgb scaled = toneMapScaleForDisplay(color.multiply(toneMapOptions.pow_bright_adjust));
        final double r = scaled.getR();
        final double g = scaled.getG();
        final double b = scaled.getB();
        return new ColorRgb(r < 0.0 ? 0.0 : (r > 1.0 ? 1.0 : r),
                            g < 0.0 ? 0.0 : (g > 1.0 ? 1.0 : g),
                            b < 0.0 ? 0.0 : (b > 1.0 ? 1.0 : b));
    }
}
