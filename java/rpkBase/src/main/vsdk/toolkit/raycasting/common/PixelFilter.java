package vsdk.toolkit.raycasting.common;

public abstract class PixelFilter {
    public PixelFilter() {
    }

    public abstract void sample(double[] xi1, double[] xi2);
}
