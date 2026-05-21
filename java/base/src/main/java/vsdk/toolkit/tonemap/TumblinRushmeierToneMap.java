package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgb;

/**
References:

[TUMB1993] J. Tumblin, H.E. Rushmeier. Tone Reproduction for Realistic Images,
IEEE Computer Graphics and Applications, 13:6, 1993, pp. 42-48.
*/
public final class TumblinRushmeierToneMap extends ToneMap {
    private float invCMaximum;
    private float lrwmComp;
    private float lrwmDisplay;
    private float lrwExponent;
    private float lda;

    public TumblinRushmeierToneMap() {
        invCMaximum = 0.0f;
        lrwmComp = 0.0f;
        lrwmDisplay = 0.0f;
        lrwExponent = 0.0f;
        lda = 0.0f;
    }

    @Override
    public void init(ToneMappingContext toneMapOptions) {
        float lwa = toneMapOptions.realWorldAdaptionLuminance;
        float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
        float maximumDisplayContrast = toneMapOptions.maximumDisplayContrast;
        lda = maximumDisplayLuminance / (float)Math.sqrt(maximumDisplayContrast);

        // Equation [COHE1993](9.16): alpha(L_w), beta(L_w) (Tumblin/Rushmeier model)
        float l10 = (float)Math.log10(tmoCandelaLambert(lwa));
        float alpha = 0.4f * l10 + 2.92f;
        float beta = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

        // Equation [COHE1993](9.16): alpha(L_d), beta(L_d)
        l10 = (float)Math.log10(tmoCandelaLambert(lda));
        float alphaD = 0.4f * l10 + 2.92f;
        float betaD = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;

        // Equation [COHE1993](9.18): L_d from L_w using adaptation-dependent exponent and scale
        lrwExponent = alpha / alphaD;
        lrwmComp = (float)Math.pow(10.0, (beta - betaD) / alphaD);
        lrwmDisplay = lrwmComp / (tmoCandelaLambert(maximumDisplayLuminance));
        invCMaximum = 1.0f / maximumDisplayContrast;
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        float rwl = Cie.spectrumLuminance(radiance.getR(), radiance.getG(), radiance.getB());

        float scale;
        if (rwl > 0.0) {
            float m = tmoLambertCandela((float)Math.pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmComp);
            scale = m > 0.0f ? m / rwl : 0.0f;
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
            float m = ((float)Math.pow(tmoCandelaLambert(rwl), lrwExponent) * lrwmDisplay - invCMaximum);
            scale = m > 0.0f ? m / rwl : 0.0f;
        }
        else {
            scale = 0.0f;
        }

        radiance.scale(scale);
        return radiance;
    }
}
