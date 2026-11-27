package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.statistics.Statistics;

public final class LightnessToneMap extends ToneMap {
    public LightnessToneMap() {
    }

    @Override
    protected void initParameters(ToneMappingContext toneMapOptions) {
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        Logger.reportMessage("LightnessToneMap", Logger.WARNING, "ScaleForComputations", String.format("%s %d not yet implemented", "LightnessToneMap.cpp", 0));
        return radiance;
    }

    @Override
    public ColorRgb scaleForDisplay(ColorRgb radiance) {
        final double max = (double)radiance.maximumComponent();
        if ( max < 1e-32 ) {
            return radiance;
        }

        // Multiply by WHITE EFFICACY to convert W/m^2sr to nits
        // (reference luminance is also in nits)
        final double scaleFactor = lightness((double)(getColorSpace().getLuminousEfficacy() * max));
        if ( scaleFactor == 0.0 ) {
            return radiance;
        }

        radiance = radiance.multiply(scaleFactor / max);
        return radiance;
    }

    private static double lightness(double luminance) {
        if ( Statistics.instance().radiance.referenceLuminance == 0.0 ) {
            return 0.0;
        }

        final double relativeLuminance = luminance / (double)Statistics.instance().radiance.referenceLuminance;
        if ( relativeLuminance > 0.008856 ) {
            return 1.16 * (double)Math.pow(relativeLuminance, 0.33) - 0.16;
        } else {
            return 9.033 * relativeLuminance;
        }
    }
}
