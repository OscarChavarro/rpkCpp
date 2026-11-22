package vsdk.toolkit.raycasting.bidirectionalRaytracing;

// Spar Config stores handy config params
public class SparConfig {
    public BidirectionalPathRaytracerConfig baseConfig;

    // Needed in weighted multi-pass methods
    public Spar leSpar;
    public Spar ldSpar;
}
