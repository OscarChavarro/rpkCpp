package vsdk.toolkit.common.color;

import java.io.PrintStream;
import java.util.Locale;

/**
Representation of radiance, radiosity, power, spectra.
*/
public class ColorRgb extends ColorRgbMutable {

    public ColorRgb() {
        this(0.0, 0.0, 0.0);
    }

    public ColorRgb(double inR, double inG, double inB) {
        super(inR, inG, inB);
    }

    public ColorRgb(ColorRgbMutable color) {
        super(color.getR(), color.getG(), color.getB());
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
            "r=" + getR() +
            ", g=" + getG() +
            ", b=" + getB() +
            '}';
    }
}
