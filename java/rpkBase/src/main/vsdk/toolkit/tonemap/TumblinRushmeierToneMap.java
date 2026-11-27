package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.ColorRgb;

/**
References:

[TUMB1993] J. Tumblin, H.E. Rushmeier. Tone Reproduction for Realistic Images,
IEEE Computer Graphics and Applications, 13:6, 1993, pp. 42-48.
*/
public final class TumblinRushmeierToneMap extends ToneMap {
    // C's M_PI literal
    private static final double M_PI = 3.14159265358979323846;

    private double invCMaximum;
    private double lrwmComp;
    private double lrwmDisplay;
    private double lrwExponent;
    private double lda;

    public TumblinRushmeierToneMap() {
        invCMaximum = 0.0;
        lrwmComp = 0.0;
        lrwmDisplay = 0.0;
        lrwExponent = 0.0;
        lda = 0.0;
    }

    @Override
    protected void initParameters(ToneMappingContext toneMapOptions) {
        final double lwa = toneMapOptions.realWorldAdaptionLuminance;
        final double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
        final double maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
        lda = maximumDisplayLuminance / (double)Math.sqrt(maximumDisplayContrast);

        // Equation [COHE1993](9.16): alpha(L_w), beta(L_w)
        double l10 = (double)Math.log10(tmoCandelaLambert(lwa));
        final double alpha = 0.4 * l10 + 2.92;
        final double beta = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;

        // Equation [COHE1993](9.16): alpha(L_d), beta(L_d)
        l10 = (double)Math.log10(tmoCandelaLambert(lda));
        final double alphaD = 0.4 * l10 + 2.92;
        final double betaD = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;

        // Equation [COHE1993](9.18): L_d from L_w using adaptation-dependent exponent and scale
        lrwExponent = alpha / alphaD;
        lrwmComp = (double)Math.pow(10.0, (beta - betaD) / alphaD);
        lrwmDisplay = lrwmComp / (tmoCandelaLambert(maximumDisplayLuminance));
        invCMaximum = 1.0 / maximumDisplayContrast;
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        final double rwl = (double)getColorSpace().spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
        double scale;

        if ( rwl > 0.0 ) {
            final double m = tmoLambertCandela(
                    (double)Math.pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmComp);
            scale = m > 0.0 ? m / rwl : 0.0;
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
            final double m = ((double)Math.pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmDisplay - invCMaximum);
            scale = m > 0.0 ? m / rwl : 0.0;
        } else {
            scale = 0.0;
        }

        radiance = radiance.multiply(scale);
        return radiance;
    }
}
