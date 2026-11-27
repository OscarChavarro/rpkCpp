package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.ColorRgb;

/**
References:

[TUMB1999b] J. Tumblin, J.K. Hodgins, B.K. Guenter. Two Methods for Display of High
Contrast Images, ACM Transactions on Graphics, 18:1, 1999, pp. 56-94.
*/
public final class RevisedTumblinRushmeierToneMap extends ToneMap {
    // C's M_PI literal
    private static final double M_PI = 3.14159265358979323846;

    private double g;
    private double comp;
    private double display;
    private double lwaRTR;
    private double ldaRTR;

    public RevisedTumblinRushmeierToneMap() {
        g = 0.0;
        comp = 0.0;
        display = 0.0;
        lwaRTR = 0.0;
        ldaRTR = 0.0;
    }

    @Override
    protected void initParameters(ToneMappingContext toneMapOptions) {
        final double lwa = toneMapOptions.realWorldAdaptionLuminance;
        final double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
        final double maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
        ldaRTR = maximumDisplayLuminance / (double)Math.sqrt(maximumDisplayContrast);

        // Equation [TUMB1999b](17): exponent gw/gd
        g = stevensGamma(lwa) / stevensGamma(ldaRTR);
        // Equation [TUMB1999b](20): gwd = gw / (1.855 + 0.4 * log10(Lda))
        final double gwd = (double)(stevensGamma(lwa) / (1.855 + 0.4 * Math.log10(ldaRTR)));
        // Equation [TUMB1999b](19): m(Lwa) * Lda
        comp = (double)Math.pow((double)Math.sqrt(maximumDisplayContrast), gwd - 1) * ldaRTR;
        display = comp / maximumDisplayLuminance;
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        final double rwl = (double)getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
        double scale;

        if ( rwl > 0.0 ) {
            // Equation [TUMB1999b](17) in multiplicative scale form
            scale = comp * (double)Math.pow(rwl / lwaRTR, g) / rwl;
        } else {
            scale = 0.0;
        }

        radiance = radiance.multiply(scale);
        return radiance;
    }

    @Override
    public ColorRgb scaleForDisplay(ColorRgb radiance) {
        final double rwl = (double)M_PI * (double)getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
        final double eff = (double)getColorSpace().getLuminousEfficacy();
        radiance = radiance.multiply(eff * (double)M_PI);

        double scale;
        if ( rwl > 0.0 ) {
            // Equation [TUMB1999b](17) in multiplicative scale form
            scale = display * (double)Math.pow(rwl / lwaRTR, g) / rwl;
        } else {
            scale = 0.0;
        }

        radiance = radiance.multiply(scale);
        return radiance;
    }

    private static double stevensGamma(double lum) {
        if ( lum > 100.0 ) {
            return 2.655;
        } else {
            // Equation [TUMB1999b](18): g(L_a)
            return (double)(1.855 + 0.4 * Math.log10(lum + 2.3e-5));
        }
    }
}
