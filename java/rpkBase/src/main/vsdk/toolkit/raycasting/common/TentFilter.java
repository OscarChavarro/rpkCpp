package vsdk.toolkit.raycasting.common;

public final class TentFilter extends PixelFilter {
    public TentFilter() {
    }

    @Override
    public void sample(double[] xi1, double[] xi2) {
        double x = Math.abs(2 * xi1[0] - 1.0);
        double sx = xi1[0] < 0.5 ? -1 : +1;
        double y = Math.abs(2 * xi2[0] - 1.0);
        double sy = xi2[0] < 0.5 ? -1 : +1;

        if (x > y) {
            xi1[0] = (sx * Math.sqrt(x)) + 0.5;
            xi2[0] = (xi1[0] * y) + 0.5;
        }
        else {
            xi2[0] = (sy * Math.sqrt(y)) + 0.5;
            xi1[0] = (xi2[0] * x) + 0.5;
        }
    }
}
