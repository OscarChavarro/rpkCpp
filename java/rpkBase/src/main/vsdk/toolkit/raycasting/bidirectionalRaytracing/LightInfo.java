package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.environment.geometry.elements.Patch;

public class LightInfo {
    public double emittedFlux;
    public double importance; // Cumulative probability : for importance sampling
    public Patch light;
}
