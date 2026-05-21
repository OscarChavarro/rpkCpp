package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;

/**
References:

[TUMB1999b] J. Tumblin, J.K. Hodgins, B.K. Guenter. Two Methods for Display of High
Contrast Images, ACM Transactions on Graphics, 18:1, 1999, pp. 56-94.
*/
public final class RevisedTumblinRushmeierToneMap extends ToneMap {
    private float g;
    private float comp;
    private float display;
    private float lwaRTR;
    private float ldaRTR;

    private static float stevensGamma(float lum) {
        if (lum > 100.0) {
            return 2.655f;
        }
        else {
            // Equation [TUMB1999b](18): g(L_a)
            return 1.855f + 0.4f * (float)Math.log10(lum + 2.3e-5f);
        }
    }

    public RevisedTumblinRushmeierToneMap() {
        g = 0.0f;
        comp = 0.0f;
        display = 0.0f;
        lwaRTR = 0.0f;
        ldaRTR = 0.0f;
    }

    @Override
    public void init(ToneMappingContext toneMapOptions) {
        float lwa = toneMapOptions.realWorldAdaptionLuminance;
        float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
        float maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
        ldaRTR = maximumDisplayLuminance / (float)Math.sqrt(maximumDisplayContrast);

        // Equation [TUMB1999b](17): exponent gw/gd
        g = stevensGamma(lwa) / stevensGamma(ldaRTR);
        // Equation [TUMB1999b](20): gwd = gw / (1.855 + 0.4 * log10(Lda))
        float gwd = stevensGamma(lwa) / (1.855f + 0.4f * (float)Math.log10(ldaRTR));
        // Equation [TUMB1999b](19): m(Lwa) * Lda
        comp = (float)Math.pow(Math.sqrt(maximumDisplayContrast), gwd - 1) * ldaRTR;
        display = comp / maximumDisplayLuminance;
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        float rwl = Cie.spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
        float scale;

        if (rwl > 0.0) {
            // Equation [TUMB1999b](17) in multiplicative scale form
            scale = (float)(comp * Math.pow(rwl / lwaRTR, g) / rwl);
        }
        else {
            scale = 0.0f;
        }

        radiance.scale(scale);
        return radiance;
    }

    @Override
    public ColorRgb scaleForDisplay(ColorRgb radiance) {
        float rwl = (float)Math.PI * Cie.spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
        float eff = Cie.getLuminousEfficacy();
        radiance.scale(eff * (float)Math.PI);

        float scale;
        if (rwl > 0.0) {
            // Equation [TUMB1999b](17) in multiplicative scale form
            scale = (float)(display * Math.pow(rwl / lwaRTR, g) / rwl);
        }
        else {
            scale = 0.0f;
        }

        radiance.scale(scale);
        return radiance;
    }
}
