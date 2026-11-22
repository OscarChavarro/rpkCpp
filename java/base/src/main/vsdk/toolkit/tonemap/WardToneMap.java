package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgbMutable;

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
        comp = 0.0F;
        display = 0.0F;
        lda = 0.0F;
    }

    @Override
    public void init(ToneMappingContext toneMapOptions) {
        final float realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
        final float maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;

        lda = maximumDisplayLuminance / 2.0F;
        final float p1 = (float) Math.pow(lda, 0.4F);
        final float p2 = (float) Math.pow(realWorldAdaptionLuminance, 0.4F);
        final float p3 = (1.219F + p1) / (1.219F + p2);
        comp = (float) Math.pow(p3, 2.5F);
        display = comp / maximumDisplayLuminance;
    }

    @Override
    public ColorRgbMutable scaleForComputations(ColorRgbMutable radiance) {
        radiance.scale(comp);
        return radiance;
    }

    @Override
    public ColorRgbMutable scaleForDisplay(ColorRgbMutable radiance) {
        final float eff = (float) Cie.getLuminousEfficacy();
        radiance.scale(eff * display);
        return radiance;
    }
}
