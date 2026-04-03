package vsdk.toolkit.common.statistics;

public class ShadowStatistics {
    public int numberOfShadowRays;
    public int numberOfShadowCacheHits;

    public ShadowStatistics() {
        numberOfShadowRays = 0;
        numberOfShadowCacheHits = 0;
    }

    public void reset() {
        numberOfShadowRays = 0;
        numberOfShadowCacheHits = 0;
    }
}
