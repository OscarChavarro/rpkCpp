package vsdk.toolkit.environment.geometry.geometricProcessing.convexPolygonClipper;

/**
Immutable clipping half-space in homogeneous coordinates: the vertices with
a * sx + b * sy + c * sz + d * sw <= 0 are inside, where (a, b, c, d) are the
x, y, z and w coefficients. A convex window is the intersection of such
half-spaces ([SUTH1974b], p. 33). For example, the screen box side x >= xMin
is (-1, 0, 0, xMin) and x <= xMax is (1, 0, 0, -xMax).
*/
public final class HomogeneousHalfSpace {
    private final double xCoefficient;
    private final double yCoefficient;
    private final double zCoefficient;
    private final double wCoefficient;

    public HomogeneousHalfSpace() {
        this(0.0, 0.0, 0.0, 0.0);
    }

    public HomogeneousHalfSpace(double inXCoefficient, double inYCoefficient, double inZCoefficient, double inWCoefficient) {
        xCoefficient = inXCoefficient;
        yCoefficient = inYCoefficient;
        zCoefficient = inZCoefficient;
        wCoefficient = inWCoefficient;
    }

    public double getXCoefficient() { return xCoefficient; }
    public double getYCoefficient() { return yCoefficient; }
    public double getZCoefficient() { return zCoefficient; }
    public double getWCoefficient() { return wCoefficient; }
}
