package vsdk.toolkit.raycasting.common;

/**
GAUSSIAN/NORMAL filter
*/
public final class NormalFilter extends PixelFilter {
    public double sigma;
    public double dist;

    public NormalFilter() {
        this(0.70710678, 2.0);
    }

    public NormalFilter(double s, double d) {
        sigma = s;
        dist = d;
    }

    @Override
    public void sample(double[] xi1, double[] xi2) {
        double s = dist / sigma;
        double r = xi1[0] * Math.exp(s * s * (-0.5));
        double a = xi2[0];

        xi1[0] = sigma * (Math.sqrt(-2.0 * Math.log(r)) * Math.cos(2.0 * Math.PI * a)) + 0.5;
        xi2[0] = sigma * (Math.sqrt(-2.0 * Math.log(r)) * Math.sin(2.0 * Math.PI * a)) + 0.5;
    }
}
