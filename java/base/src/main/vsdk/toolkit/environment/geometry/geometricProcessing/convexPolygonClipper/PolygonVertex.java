package vsdk.toolkit.environment.geometry.geometricProcessing.convexPolygonClipper;

/**
Immutable polygon vertex: homogeneous screen space position (sx, sy, sz, sw),
world space position (x, y, z), texture position (u, v) and color (r, g, b).
Each attribute also has an index, in that order, to select it by number in
the interpolation masks of ScreenPolygon and in getCoord.
*/
public final class PolygonVertex {
    public static final int SX = 0;
    public static final int SY = 1;
    public static final int SZ = 2;
    public static final int SW = 3;
    public static final int X = 4;
    public static final int Y = 5;
    public static final int Z = 6;
    public static final int U = 7;
    public static final int V = 8;
    public static final int R = 9;
    public static final int G = 10;
    public static final int B = 11;
    public static final int NUMBER_OF_ATTRIBUTES = 12;

    private final double[] attributes;

    public PolygonVertex() {
        attributes = new double[NUMBER_OF_ATTRIBUTES];
    }

    public PolygonVertex(
        double inSx, double inSy, double inSz, double inSw,
        double inX, double inY, double inZ,
        double inU, double inV,
        double inR, double inG, double inB) {
        attributes = new double[] {inSx, inSy, inSz, inSw, inX, inY, inZ, inU, inV, inR, inG, inB};
    }

    /** Vertex with the NUMBER_OF_ATTRIBUTES attributes given in index order */
    public static PolygonVertex
    fromAttributes(double[] values) {
        return new PolygonVertex(values[SX], values[SY], values[SZ], values[SW],
                                 values[X], values[Y], values[Z],
                                 values[U], values[V],
                                 values[R], values[G], values[B]);
    }

    public double getSx() { return attributes[SX]; }
    public double getSy() { return attributes[SY]; }
    public double getSz() { return attributes[SZ]; }
    public double getSw() { return attributes[SW]; }
    public double getX() { return attributes[X]; }
    public double getY() { return attributes[Y]; }
    public double getZ() { return attributes[Z]; }
    public double getU() { return attributes[U]; }
    public double getV() { return attributes[V]; }
    public double getR() { return attributes[R]; }
    public double getG() { return attributes[G]; }
    public double getB() { return attributes[B]; }

    /** The attribute of the given index, or 0.0 outside [0, NUMBER_OF_ATTRIBUTES) */
    public double
    getCoord(int index) {
        return index >= 0 && index < NUMBER_OF_ATTRIBUTES ? attributes[index] : 0.0;
    }
}
