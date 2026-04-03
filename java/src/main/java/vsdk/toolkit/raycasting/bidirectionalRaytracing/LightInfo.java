package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.skin.Patch;

public class LightInfo {
    public float emittedFlux;
    public float importance; // Cumulative probability : for importance sampling
    public Patch light;
}
