package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;

public final class SparContribution {
    private final ColorRgb radiance;
    private final ColorRgb bptRadiance;

    public SparContribution(ColorRgb radiance, ColorRgb bptRadiance) {
        this.radiance = radiance;
        this.bptRadiance = bptRadiance;
    }

    public ColorRgb getRadiance() {
        return radiance;
    }

    public ColorRgb getBptRadiance() {
        return bptRadiance;
    }
}
