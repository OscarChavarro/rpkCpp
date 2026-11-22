package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgbMutable;

/**
References:

[TUMB1999b] J. Tumblin, J.K. Hodgins, B.K. Guenter. Two Methods for Display of High
Contrast Images, ACM Transactions on Graphics, 18:1, 1999, pp. 56-94.
*/
public final class RevisedTumblinRushmeierToneMap extends ToneMap {
    // C's M_PI literal
    private static final double M_PI = 3.14159265358979323846;

    private float g;
    private float comp;
    private float display;
    private float lwaRTR;
    private float ldaRTR;

    public RevisedTumblinRushmeierToneMap() {
        g = 0.0F;
        comp = 0.0F;
        display = 0.0F;
        lwaRTR = 0.0F;
        ldaRTR = 0.0F;
    }

    @Override
    public void init(ToneMappingContext toneMapOptions) {
        final float lwa = toneMapOptions.realWorldAdaptionLuminance;
        final float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
        final float maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
        ldaRTR = maximumDisplayLuminance / (float) Math.sqrt(maximumDisplayContrast);

        // Equation [TUMB1999b](17): exponent gw/gd
        g = stevensGamma(lwa) / stevensGamma(ldaRTR);
        // Equation [TUMB1999b](20): gwd = gw / (1.855 + 0.4 * log10(Lda))
        final float gwd = (float) (stevensGamma(lwa) / (1.855F + 0.4F * Math.log10(ldaRTR)));
        // Equation [TUMB1999b](19): m(Lwa) * Lda
        comp = (float) Math.pow((float) Math.sqrt(maximumDisplayContrast), gwd - 1) * ldaRTR;
        display = comp / maximumDisplayLuminance;
    }

    @Override
    public ColorRgbMutable scaleForComputations(ColorRgbMutable radiance) {
        final float rwl = (float) Cie.spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
        float scale;

        if ( rwl > 0.0 ) {
            // Equation [TUMB1999b](17) in multiplicative scale form
            scale = comp * (float) Math.pow(rwl / lwaRTR, g) / rwl;
        } else {
            scale = 0.0F;
        }

        radiance.scale(scale);
        return radiance;
    }

    @Override
    public ColorRgbMutable scaleForDisplay(ColorRgbMutable radiance) {
        final float rwl = (float) M_PI * (float) Cie.spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
        final float eff = (float) Cie.getLuminousEfficacy();
        radiance.scale(eff * (float) M_PI);

        float scale;
        if ( rwl > 0.0 ) {
            // Equation [TUMB1999b](17) in multiplicative scale form
            scale = display * (float) Math.pow(rwl / lwaRTR, g) / rwl;
        } else {
            scale = 0.0F;
        }

        radiance.scale(scale);
        return radiance;
    }

    private static float stevensGamma(float lum) {
        if ( lum > 100.0 ) {
            return 2.655F;
        } else {
            // Equation [TUMB1999b](18): g(L_a)
            return (float) (1.855F + 0.4F * Math.log10(lum + 2.3e-5F));
        }
    }
}
