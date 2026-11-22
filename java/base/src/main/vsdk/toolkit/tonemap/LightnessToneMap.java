package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.statistics.Statistics;

public final class LightnessToneMap extends ToneMap {
    public LightnessToneMap() {
    }

    @Override
    public void init(ToneMappingContext toneMapOptions) {
    }

    @Override
    public ColorRgbMutable scaleForComputations(ColorRgbMutable radiance) {
        Logger.reportMessage("LightnessToneMap", Logger.WARNING, "ScaleForComputations", String.format("%s %d not yet implemented", "LightnessToneMap.cpp", 0));
        return radiance;
    }

    @Override
    public ColorRgbMutable scaleForDisplay(ColorRgbMutable radiance) {
        final float max = (float) radiance.maximumComponent();
        if ( max < 1e-32 ) {
            return radiance;
        }

        // Multiply by WHITE EFFICACY to convert W/m^2sr to nits
        // (reference luminance is also in nits)
        final float scaleFactor = lightness((float) (Cie.WHITE_EFFICACY * max));
        if ( scaleFactor == 0.0 ) {
            return radiance;
        }

        radiance.scale(scaleFactor / max);
        return radiance;
    }

    private static float lightness(float luminance) {
        if ( Statistics.instance().radiance.referenceLuminance == 0.0 ) {
            return 0.0F;
        }

        final float relativeLuminance = luminance / (float) Statistics.instance().radiance.referenceLuminance;
        if ( relativeLuminance > 0.008856 ) {
            return 1.16F * (float) Math.pow(relativeLuminance, 0.33F) - 0.16F;
        } else {
            return 9.033F * relativeLuminance;
        }
    }
}
