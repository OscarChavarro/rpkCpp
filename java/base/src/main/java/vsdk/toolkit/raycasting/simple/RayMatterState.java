package vsdk.toolkit.raycasting.simple;

public class RayMatterState {
    public int samplesPerPixel; // Pixel sampling
    public RayMatterFilterType filter; // Pixel filter

    public RayMatterState() {
        samplesPerPixel = 8;
        filter = RayMatterFilterType.TENT_FILTER;
    }
}
