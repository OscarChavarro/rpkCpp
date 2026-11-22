package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgbMutable;

/**
References:

[TUMB1993] J. Tumblin, H.E. Rushmeier. Tone Reproduction for Realistic Images,
IEEE Computer Graphics and Applications, 13:6, 1993, pp. 42-48.
*/
public final class TumblinRushmeierToneMap extends ToneMap {
    // C's M_PI literal
    private static final double M_PI = 3.14159265358979323846;

    private float invCMaximum;
    private float lrwmComp;
    private float lrwmDisplay;
    private float lrwExponent;
    private float lda;

    public TumblinRushmeierToneMap() {
        invCMaximum = 0.0F;
        lrwmComp = 0.0F;
        lrwmDisplay = 0.0F;
        lrwExponent = 0.0F;
        lda = 0.0F;
    }

    @Override
    public void init(ToneMappingContext toneMapOptions) {
        final float lwa = toneMapOptions.realWorldAdaptionLuminance;
        final float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
        final float maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
        lda = maximumDisplayLuminance / (float) Math.sqrt(maximumDisplayContrast);

        // Equation [COHE1993](9.16): alpha(L_w), beta(L_w)
        float l10 = (float) Math.log10(tmoCandelaLambert(lwa));
        final float alpha = 0.4F * l10 + 2.92F;
        final float beta = -0.4F * (l10 * l10) - 2.584F * l10 + 2.0208F;

        // Equation [COHE1993](9.16): alpha(L_d), beta(L_d)
        l10 = (float) Math.log10(tmoCandelaLambert(lda));
        final float alphaD = 0.4F * l10 + 2.92F;
        final float betaD = -0.4F * (l10 * l10) - 2.584F * l10 + 2.0208F;

        // Equation [COHE1993](9.18): L_d from L_w using adaptation-dependent exponent and scale
        lrwExponent = alpha / alphaD;
        lrwmComp = (float) Math.pow(10.0F, (beta - betaD) / alphaD);
        lrwmDisplay = lrwmComp / (tmoCandelaLambert(maximumDisplayLuminance));
        invCMaximum = 1.0F / maximumDisplayContrast;
    }

    @Override
    public ColorRgbMutable scaleForComputations(ColorRgbMutable radiance) {
        final float rwl = (float) Cie.spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());
        float scale;

        if ( rwl > 0.0 ) {
            final float m = tmoLambertCandela(
                    (float) Math.pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmComp);
            scale = m > 0.0F ? m / rwl : 0.0F;
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
            final float m = ((float) Math.pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmDisplay - invCMaximum);
            scale = m > 0.0F ? m / rwl : 0.0F;
        } else {
            scale = 0.0F;
        }

        radiance.scale(scale);
        return radiance;
    }
}
