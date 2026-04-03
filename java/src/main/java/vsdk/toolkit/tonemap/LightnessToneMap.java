package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.Cie;
import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.statistics.Statistics;

public final class LightnessToneMap extends ToneMap {
    private static float lightness(float luminance) {
        if (Statistics.instance().radiance.referenceLuminance == 0.0) {
            return 0.0f;
        }

        float relativeLuminance = luminance / (float)Statistics.instance().radiance.referenceLuminance;
        if (relativeLuminance > 0.008856) {
            return 1.16f * (float)Math.pow(relativeLuminance, 0.33) - 0.16f;
        }
        else {
            return 9.033f * relativeLuminance;
        }
    }

    public LightnessToneMap() {
    }

    /*toneMapOptions*/
    @Override
    public void init(ToneMappingContext toneMapOptions) {
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        Error.warning("ScaleForComputations", "%s %d not yet implemented", "LightnessToneMap.cpp", 0);
        return radiance;
    }

    @Override
    public ColorRgb scaleForDisplay(ColorRgb radiance) {
        float max = radiance.maximumComponent();
        if (max < 1e-32) {
            return radiance;
        }

        // Multiply by WHITE EFFICACY to convert W/m^2sr to nits
        // (reference luminance is also in nits)
        float scaleFactor = lightness(Cie.WHITE_EFFICACY * max);
        if (scaleFactor == 0.0) {
            return radiance;
        }

        radiance.scale(scaleFactor / max);
        return radiance;
    }
}
