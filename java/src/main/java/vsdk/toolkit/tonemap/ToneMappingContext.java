package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.Cie;
import vsdk.toolkit.common.ColorRgb;

public class ToneMappingContext {
    // Gamma correction table
    public static final int GAMMA_TABLE_BITS = 12;
    public static final int GAMMA_TABLE_SIZE = (1 << GAMMA_TABLE_BITS) + 1;

    // Fixed radiance rescaling before tone mapping
    public float brightness_adjust; // Brightness adjustment factor
    public float pow_bright_adjust; // pow(2, brightness_adjust)

    // Variable / non-linear radiance rescaling parameters
    public ToneMapAdaptationMethod staticAdaptationMethod;
    public float realWorldAdaptionLuminance;
    public float maximumDisplayLuminance;
    public float maximumDisplayContrast;

    // Conversion from radiance (COLOR type) to display RGB
    public float xr; // Monitor primary colors
    public float yr;
    public float xg;
    public float yg;
    public float xb;
    public float yb;
    public float xw; // Monitor white point
    public float yw;

    // Display RGB mapping (corrects display non-linear response)
    public ColorRgb gamma; // Gamma factors for red, green, blue
    public float[][] gammaTab; // Gamma correction tables for red, green and blue

    private static final float DEFAULT_GAMMA = 1.7f;
    private static final float DEFAULT_TM_LWA = 10.0f;
    private static final float DEFAULT_TM_LD_MAXIMUM = 100.0f;
    private static final float DEFAULT_TM_C_MAXIMUM = 50.0f;

    public ToneMappingContext() {
        brightness_adjust = 0.0f;
        pow_bright_adjust = (float)Math.pow(2.0, brightness_adjust);

        staticAdaptationMethod = ToneMapAdaptationMethod.TMA_MEDIAN;
        realWorldAdaptionLuminance = DEFAULT_TM_LWA;
        maximumDisplayLuminance = DEFAULT_TM_LD_MAXIMUM;
        maximumDisplayContrast = DEFAULT_TM_C_MAXIMUM;

        xr = 0.640f;
        yr = 0.330f;
        xg = 0.290f;
        yg = 0.600f;
        xb = 0.150f;
        yb = 0.060f;
        xw = 0.333333333333f;
        yw = 0.333333333333f;
        Cie.computeColorConversionTransforms(xr, yr, xg, yg, xb, yb, xw, yw);

        gamma = new ColorRgb();
        gamma.set(DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA);
        gammaTab = new float[3][GAMMA_TABLE_SIZE];
        ToneMap.recomputeGammaTables(this, gamma);
    }
}
