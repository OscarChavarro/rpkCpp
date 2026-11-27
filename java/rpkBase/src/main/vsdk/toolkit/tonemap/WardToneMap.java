package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.ColorRgb;

/**
References:

G. Ward. A Contrast-Based Scale factor for Luminance Display, Graphics
Gems IV, Academic Press, 1994, pp. 415-421.
*/
public final class WardToneMap extends ToneMap {
    private double comp;
    private double display;
    private double lda;

    public WardToneMap() {
        comp = 0.0;
        display = 0.0;
        lda = 0.0;
    }

    @Override
    protected void initParameters(ToneMappingContext toneMapOptions) {
        final double realWorldAdaptionLuminance = toneMapOptions.realWorldAdaptionLuminance;
        final double maximumDisplayLuminance = toneMapOptions.maximumDisplayLuminance;

        lda = maximumDisplayLuminance / 2.0;
        final double p1 = (double)Math.pow(lda, 0.4);
        final double p2 = (double)Math.pow(realWorldAdaptionLuminance, 0.4);
        final double p3 = (1.219 + p1) / (1.219 + p2);
        comp = (double)Math.pow(p3, 2.5);
        display = comp / maximumDisplayLuminance;
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        radiance = radiance.multiply(comp);
        return radiance;
    }

    @Override
    public ColorRgb scaleForDisplay(ColorRgb radiance) {
        final double eff = (double)getColorSpace().getLuminousEfficacy();
        radiance = radiance.multiply(eff * display);
        return radiance;
    }
}
