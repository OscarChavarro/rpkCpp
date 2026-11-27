package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;

public class ToneMappingContext {
    // Gamma correction table
    public static final int GAMMA_TABLE_BITS = 12;
    public static final int GAMMA_TABLE_SIZE = (1 << GAMMA_TABLE_BITS) + 1;

    // Fixed radiance rescaling before tone mapping
    public double brightness_adjust; // Brightness adjustment factor
    public double pow_bright_adjust; // pow(2, brightness_adjust)

    // Variable / non-linear radiance rescaling parameters
    public ToneMapAdaptationMethod staticAdaptationMethod;
    public double realWorldAdaptionLuminance;
    public double maximumDisplayLuminance;
    public double maximumDisplayContrast;


    // Display RGB mapping (corrects display non-linear response)
    public ColorRgb gamma; // Gamma factors for red, green, blue
    public double[][] gammaTab; // Gamma correction tables for red, green and blue

    // Conversion from radiance to display RGB
    private Cie colorSpace;

    private static final double DEFAULT_GAMMA = 1.7;
    private static final double DEFAULT_TM_LWA = 10.0;
    private static final double DEFAULT_TM_LD_MAXIMUM = 100.0;
    private static final double DEFAULT_TM_C_MAXIMUM = 50.0;

    public ToneMappingContext() {
        brightness_adjust = 0.0;
        pow_bright_adjust = (double)Math.pow(2.0, brightness_adjust);

        staticAdaptationMethod = ToneMapAdaptationMethod.TMA_MEDIAN;
        realWorldAdaptionLuminance = DEFAULT_TM_LWA;
        maximumDisplayLuminance = DEFAULT_TM_LD_MAXIMUM;
        maximumDisplayContrast = DEFAULT_TM_C_MAXIMUM;

        // Monitor primaries and white point
        colorSpace = new Cie(
            0.640, 0.330,
            0.290, 0.600,
            0.150, 0.060,
            0.333333333333, 0.333333333333);

        gamma = new ColorRgb(DEFAULT_GAMMA, DEFAULT_GAMMA, DEFAULT_GAMMA);
        gammaTab = new double[3][GAMMA_TABLE_SIZE];
        ToneMap.recomputeGammaTables(this, gamma);
    }

    public Cie getColorSpace() {
        return colorSpace;
    }

    public void setColorSpace(Cie inColorSpace) {
        colorSpace = inColorSpace;
    }
}
