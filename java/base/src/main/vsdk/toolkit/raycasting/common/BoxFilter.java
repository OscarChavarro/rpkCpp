package vsdk.toolkit.raycasting.common;

public final class BoxFilter extends PixelFilter {
    public BoxFilter() {
    }

    @Override
    public void sample(double[] dx, double[] dy) {
        // Box filter means not changing original (dx, dy) point.
    }
}
