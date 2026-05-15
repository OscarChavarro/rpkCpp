package vsdk.toolkit.common.color;

/**
Representation of radiance, radiosity, power, spectra
*/

import java.io.PrintStream;
import java.util.Locale;

public class ColorRgb {
    public double r;
    public double g;
    public double b;

    public ColorRgb() {
        this(0.0, 0.0, 0.0);
    }

    public ColorRgb(double inR, double inG, double inB) {
        this.r = inR;
        this.g = inG;
        this.b = inB;
    }

    public void clear() {
        r = 0.0;
        g = 0.0;
        b = 0.0;
    }

    public void set(double v1, double v2, double v3) {
        r = v1;
        g = v2;
        b = v3;
    }

    public void setMonochrome(double v) {
        r = v;
        g = v;
        b = v;
    }

    public boolean isBlack() {
        return (r > -vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON && r < vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON &&
            g > -vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON && g < vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON &&
            b > -vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON && b < vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON);
    }

    public void scaledCopy(double a, ColorRgb c) {
        r = a * c.r;
        g = a * c.g;
        b = a * c.b;
    }

    public void scale(double a) {
        r *= a;
        g *= a;
        b *= a;
    }

    public void scalarProduct(ColorRgb s, ColorRgb t) {
        r = s.r * t.r;
        g = s.g * t.g;
        b = s.b * t.b;
    }

    public void selfScalarProduct(ColorRgb s) {
        r *= s.r;
        g *= s.g;
        b *= s.b;
    }

    public void scalarProductScaled(ColorRgb s, double a, ColorRgb t) {
        r = s.r * a * t.r;
        g = s.g * a * t.g;
        b = s.b * a * t.b;
    }

    public void add(ColorRgb s, ColorRgb t) {
        r = s.r + t.r;
        g = s.g + t.g;
        b = s.b + t.b;
    }

    public void addScaled(ColorRgb s, double a, ColorRgb t) {
        r = s.r + a * t.r;
        g = s.g + a * t.g;
        b = s.b + a * t.b;
    }

    public void addConstant(ColorRgb s, double a) {
        r = s.r + a;
        g = s.g + a;
        b = s.b + a;
    }

    public void subtract(ColorRgb s, ColorRgb t) {
        r = s.r - t.r;
        g = s.g - t.g;
        b = s.b - t.b;
    }

    public void divide(ColorRgb s, ColorRgb t) {
        r = (t.r != 0.0) ? s.r / t.r : s.r;
        g = (t.g != 0.0) ? s.g / t.g : s.g;
        b = (t.b != 0.0) ? s.b / t.b : s.b;
    }

    public void scaleInverse(double scale, ColorRgb s) {
        double a = (scale != 0.0) ? 1.0 / scale : 1.0;
        r = a * s.r;
        g = a * s.g;
        b = a * s.b;
    }

    public float maximumComponent() {
        if (r > g) {
            return (float)((r > b) ? r : b);
        }
        return (float)((g > b) ? g : b);
    }

    public double sumAbsComponents() {
        return Math.abs(r) + Math.abs(g) + Math.abs(b);
    }

    public void abs() {
        r = Math.abs(r);
        g = Math.abs(g);
        b = Math.abs(b);
    }

    public void maximum(ColorRgb s, ColorRgb t) {
        r = (s.r > t.r) ? s.r : t.r;
        g = (s.g > t.g) ? s.g : t.g;
        b = (s.b > t.b) ? s.b : t.b;
    }

    public void minimum(ColorRgb s, ColorRgb t) {
        r = (s.r < t.r) ? s.r : t.r;
        g = (s.g < t.g) ? s.g : t.g;
        b = (s.b < t.b) ? s.b : t.b;
    }

    public float average() {
        return (float)((r + g + b) / 3.0);
    }

    public void interpolateBarycentric(ColorRgb c0, ColorRgb c1, ColorRgb c2, double u, double v) {
        r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
        g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
        b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
    }

    public void interpolateBiLinear(ColorRgb c0, ColorRgb c1, ColorRgb c2, ColorRgb c3, double u, double v) {
        double c = u * v;
        double bb = u - c;
        double d = v - c;

        r = c0.r + bb * (c1.r - c0.r) + c * (c2.r - c0.r) + d * (c3.r - c0.r);
        g = c0.g + bb * (c1.g - c0.g) + c * (c2.g - c0.g) + d * (c3.g - c0.g);
        b = c0.b + bb * (c1.b - c0.b) + c * (c2.b - c0.b) + d * (c3.b - c0.b);
    }

    public void clip() {
        if (r < 0.0) {
            r = 0.0;
        }
        else if (r > 1.0) {
            r = 1.0;
        }

        if (g < 0.0) {
            g = 0.0;
        }
        else if (g > 1.0) {
            g = 1.0;
        }

        if (b < 0.0) {
            b = 0.0;
        }
        else if (b > 1.0) {
            b = 1.0;
        }
    }

    public void print(PrintStream stream) {
        if (stream == null) {
            return;
        }
        stream.printf(Locale.US, "%g %g %g", r, g, b);
    }

    public static void arrayCopy(ColorRgb[] result, ColorRgb[] source, int n) {
        for (int i = 0; i < n; i++) {
            if (result[i] == null) {
                result[i] = new ColorRgb();
            }
            if (source[i] == null) {
                result[i].clear();
            }
            else {
                result[i].set(source[i].r, source[i].g, source[i].b);
            }
        }
    }

    public static void arrayAdd(ColorRgb[] result, ColorRgb[] source, int n) {
        for (int i = 0; i < n; i++) {
            result[i].add(result[i], source[i]);
        }
    }

    public static void arrayClear(ColorRgb[] color, int n) {
        for (int i = 0; i < n; i++) {
            color[i].clear();
        }
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
