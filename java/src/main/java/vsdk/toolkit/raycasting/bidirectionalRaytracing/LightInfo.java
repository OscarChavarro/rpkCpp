package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.environment.geometry.elements.Patch;

public class LightInfo {
    public float emittedFlux;
    public float importance; // Cumulative probability : for importance sampling
    public Patch light;
}
