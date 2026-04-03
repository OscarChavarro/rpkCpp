package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.Cie;
import vsdk.toolkit.common.ColorRgb;

/**
References:

G. Ward. A Contrast-Based Scale factor for Luminance Display, Graphics
Gems IV, Academic Press, 1994, pp. 415-421.
*/
public final class WardToneMap extends ToneMap {
    private float comp;
    private float display;
    private float lda;

    public WardToneMap() {
        comp = 0.0f;
        display = 0.0f;
        lda = 0.0f;
    }

    @Override
    public void init(ToneMappingContext toneMapOptions) {
        float realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
        float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;
        lda = maximumDisplayLuminance / 2.0f;

        float p1 = (float)Math.pow(lda, 0.4);
        float p2 = (float)Math.pow(realWorldAdaptionLuminance, 0.4);
        float p3 = (1.219f + p1) / (1.219f + p2);
        comp = (float)Math.pow(p3, 2.5);
        display = comp / maximumDisplayLuminance;
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        radiance.scale(comp);
        return radiance;
    }

    @Override
    public ColorRgb scaleForDisplay(ColorRgb radiance) {
        float eff = Cie.getLuminousEfficacy();

        radiance.scale(eff * display);
        return radiance;
    }
}
