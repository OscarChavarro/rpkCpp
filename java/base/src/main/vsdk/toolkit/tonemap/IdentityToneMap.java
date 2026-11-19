package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.color.ColorRgbMutable;

public final class IdentityToneMap extends ToneMap {
    public IdentityToneMap() {
    }

    /*toneMapOptions*/
    @Override
    public void init(ToneMappingContext toneMapOptions) {
    }

    @Override
    public ColorRgbMutable scaleForComputations(ColorRgbMutable radiance) {
        return radiance;
    }

    @Override
    public ColorRgbMutable scaleForDisplay(ColorRgbMutable radiance) {
        return radiance;
    }
}
