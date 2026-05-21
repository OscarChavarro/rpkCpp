package vsdk.toolkit.common.color;

import java.io.PrintStream;
import java.util.Locale;

/**
Representation of radiance, radiosity, power, spectra.
Immutable variant.
*/
public class ColorRgb {
    private final double r;
    private final double g;
    private final double b;

    public ColorRgb() {
        this(0.0, 0.0, 0.0);
    }

    public ColorRgb(double inR, double inG, double inB) {
        this.r = inR;
        this.g = inG;
        this.b = inB;
    }

    public ColorRgb(ColorRgbMutable color) {
        this(color.r, color.g, color.b);
    }

    public double getR() { return r; }
    public double getG() { return g; }
    public double getB() { return b; }

    public boolean isBlack() {
        return (r > -vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON && r < vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON &&
            g > -vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON && g < vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON &&
            b > -vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON && b < vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON);
    }

    public float maximumComponent() {
        if (r > g) {
            return (float) ((r > b) ? r : b);
        }
        return (float) ((g > b) ? g : b);
    }

    public double sumAbsComponents() {
        return Math.abs(r) + Math.abs(g) + Math.abs(b);
    }

    public float average() {
        return (float) ((r + g + b) / 3.0);
    }

    public void print(PrintStream stream) {
        if (stream == null) {
            return;
        }
        stream.printf(Locale.US, "%g %g %g", r, g, b);
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
