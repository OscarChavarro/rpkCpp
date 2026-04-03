package vsdk.toolkit.tonemap;

import vsdk.toolkit.common.ColorRgb;

public final class IdentityToneMap extends ToneMap {
    public IdentityToneMap() {
    }

    /*toneMapOptions*/
    @Override
    public void init(ToneMappingContext toneMapOptions) {
    }

    @Override
    public ColorRgb scaleForComputations(ColorRgb radiance) {
        return radiance;
    }

    @Override
    public ColorRgb scaleForDisplay(ColorRgb radiance) {
        return radiance;
    }
}
