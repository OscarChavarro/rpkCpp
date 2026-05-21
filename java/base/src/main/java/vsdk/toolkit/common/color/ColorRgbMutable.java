package vsdk.toolkit.common.color;

import java.io.PrintStream;
import java.util.Locale;

/**
 Mutable RGB color for iterative algorithms.
*/
public class ColorRgbMutable {
    public double r;
    public double g;
    public double b;

    public ColorRgbMutable() {
        this(0.0, 0.0, 0.0);
    }

    public ColorRgbMutable(double inR, double inG, double inB) {
        this.r = inR;
        this.g = inG;
        this.b = inB;
    }

    public ColorRgbMutable(ColorRgb c) {
        this(c.getR(), c.getG(), c.getB());
    }

    public void clear() { r = 0.0; g = 0.0; b = 0.0; }
    public void set(double v1, double v2, double v3) { r = v1; g = v2; b = v3; }
    public void setMonochrome(double v) { r = v; g = v; b = v; }

    public boolean isBlack() {
        return (r > -vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON && r < vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON &&
            g > -vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON && g < vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON &&
            b > -vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON && b < vsdk.toolkit.common.linealAlgebra.Numeric.EPSILON);
    }

    public void scaledCopy(double a, ColorRgbMutable c) { r = a * c.r; g = a * c.g; b = a * c.b; }
    public void scale(double a) { r *= a; g *= a; b *= a; }
    public void scalarProduct(ColorRgbMutable s, ColorRgbMutable t) { r = s.r * t.r; g = s.g * t.g; b = s.b * t.b; }
    public void selfScalarProduct(ColorRgbMutable s) { r *= s.r; g *= s.g; b *= s.b; }
    public void scalarProductScaled(ColorRgbMutable s, double a, ColorRgbMutable t) { r = s.r * a * t.r; g = s.g * a * t.g; b = s.b * a * t.b; }
    public void add(ColorRgbMutable s, ColorRgbMutable t) { r = s.r + t.r; g = s.g + t.g; b = s.b + t.b; }
    public void addScaled(ColorRgbMutable s, double a, ColorRgbMutable t) { r = s.r + a * t.r; g = s.g + a * t.g; b = s.b + a * t.b; }
    public void addConstant(ColorRgbMutable s, double a) { r = s.r + a; g = s.g + a; b = s.b + a; }
    public void subtract(ColorRgbMutable s, ColorRgbMutable t) { r = s.r - t.r; g = s.g - t.g; b = s.b - t.b; }
    public void divide(ColorRgbMutable s, ColorRgbMutable t) {
        r = (t.r != 0.0) ? s.r / t.r : s.r;
        g = (t.g != 0.0) ? s.g / t.g : s.g;
        b = (t.b != 0.0) ? s.b / t.b : s.b;
    }

    public void scaleInverse(double scale, ColorRgbMutable s) {
        double a = (scale != 0.0) ? 1.0 / scale : 1.0;
        r = a * s.r;
        g = a * s.g;
        b = a * s.b;
    }

    public float maximumComponent() {
        if (r > g) return (float)((r > b) ? r : b);
        return (float)((g > b) ? g : b);
    }

    public double sumAbsComponents() { return Math.abs(r) + Math.abs(g) + Math.abs(b); }
    public void abs() { r = Math.abs(r); g = Math.abs(g); b = Math.abs(b); }
    public void maximum(ColorRgbMutable s, ColorRgbMutable t) { r = (s.r > t.r) ? s.r : t.r; g = (s.g > t.g) ? s.g : t.g; b = (s.b > t.b) ? s.b : t.b; }
    public void minimum(ColorRgbMutable s, ColorRgbMutable t) { r = (s.r < t.r) ? s.r : t.r; g = (s.g < t.g) ? s.g : t.g; b = (s.b < t.b) ? s.b : t.b; }
    public float average() { return (float)((r + g + b) / 3.0); }
    public void interpolateBarycentric(ColorRgbMutable c0, ColorRgbMutable c1, ColorRgbMutable c2, double u, double v) {
        r = c0.r + u * (c1.r - c0.r) + v * (c2.r - c0.r);
        g = c0.g + u * (c1.g - c0.g) + v * (c2.g - c0.g);
        b = c0.b + u * (c1.b - c0.b) + v * (c2.b - c0.b);
    }

    public void interpolateBiLinear(ColorRgbMutable c0, ColorRgbMutable c1, ColorRgbMutable c2, ColorRgbMutable c3, double u, double v) {
        double c = u * v;
        double bb = u - c;
        double d = v - c;
        r = c0.r + bb * (c1.r - c0.r) + c * (c2.r - c0.r) + d * (c3.r - c0.r);
        g = c0.g + bb * (c1.g - c0.g) + c * (c2.g - c0.g) + d * (c3.g - c0.g);
        b = c0.b + bb * (c1.b - c0.b) + c * (c2.b - c0.b) + d * (c3.b - c0.b);
    }

    public void clip() {
        if (r < 0.0) r = 0.0; else if (r > 1.0) r = 1.0;
        if (g < 0.0) g = 0.0; else if (g > 1.0) g = 1.0;
        if (b < 0.0) b = 0.0; else if (b > 1.0) b = 1.0;
    }

    public void print(PrintStream stream) {
        if (stream == null) return;
        stream.printf(Locale.US, "%g %g %g", r, g, b);
    }

    public static void arrayCopy(ColorRgbMutable[] result, ColorRgbMutable[] source, int n) {
        for (int i = 0; i < n; i++) {
            if (result[i] == null) result[i] = new ColorRgbMutable();
            if (source[i] == null) result[i].clear();
            else result[i].set(source[i].r, source[i].g, source[i].b);
        }
    }

    public static void arrayAdd(ColorRgbMutable[] result, ColorRgbMutable[] source, int n) {
        for (int i = 0; i < n; i++) result[i].add(result[i], source[i]);
    }

    public static void arrayClear(ColorRgbMutable[] color, int n) {
        for (int i = 0; i < n; i++) color[i].clear();
    }

    public ColorRgb toImmutable() {
        return new ColorRgb(r, g, b);
    }
}
