package vsdk.toolkit.common;

public class ColorRgb {
    public double r;
    public double g;
    public double b;

    public ColorRgb() {
        this(0.0, 0.0, 0.0);
    }

    public ColorRgb(double r, double g, double b) {
        this.r = r;
        this.g = g;
        this.b = b;
    }

    public void set(double r, double g, double b) {
        this.r = r;
        this.g = g;
        this.b = b;
    }

    @Override
    public String toString() {
        return "ColorRgb{" +
            "r=" + r +
            ", g=" + g +
            ", b=" + b +
            '}';
    }
}
