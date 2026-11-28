package vsdk.toolkit.environment.geometry.geometricProcessing.convexPolygonClipper;

/**
Immutable convex polygon with homogeneous vertices, as clipped by
ConvexWindowClipper. The mask is an interpolation mask whose kth bit
indicates whether the kth attribute of a PolygonVertex is relevant. For
example, if the valid attributes are sx, sy, and sz, then set
mask = ScreenPolygon.attributeBit(PolygonVertex.SX) |
       ScreenPolygon.attributeBit(PolygonVertex.SY) |
       ScreenPolygon.attributeBit(PolygonVertex.SZ);
*/
public final class ScreenPolygon {
    // Clipping an n-gon against k planes can give an (n + k)-gon
    public static final int MAXIMUM_SIDES_PER_POLYGON = 10;

    private final int numberOfVertices;
    private final long mask; // Interpolation mask for vertex elems
    private final PolygonVertex[] vertices;

    /** The empty polygon */
    public ScreenPolygon() {
        numberOfVertices = 0;
        mask = 0L;
        vertices = newEmptyVertices();
    }

    /** Copies the first inNumberOfVertices vertices of inVertices */
    public ScreenPolygon(int inNumberOfVertices, long inMask, PolygonVertex[] inVertices) {
        numberOfVertices = inNumberOfVertices;
        mask = inMask;
        vertices = newEmptyVertices();
        if ( inNumberOfVertices < 0 || inNumberOfVertices > MAXIMUM_SIDES_PER_POLYGON ) {
            System.err.printf("ScreenPolygon: invalid number of vertices: %d (max=%d)\n",
                inNumberOfVertices, MAXIMUM_SIDES_PER_POLYGON);
            System.exit(1);
        }
        for ( int i = 0; i < inNumberOfVertices; i++ ) {
            vertices[i] = inVertices[i];
        }
    }

    /**
    Default constructed vertices, as the value initialized C++ array of
    PolygonVertex
    */
    private static PolygonVertex[] newEmptyVertices() {
        PolygonVertex[] emptyVertices = new PolygonVertex[MAXIMUM_SIDES_PER_POLYGON];
        for ( int i = 0; i < MAXIMUM_SIDES_PER_POLYGON; i++ ) {
            emptyVertices[i] = new PolygonVertex();
        }
        return emptyVertices;
    }

    public static long
    attributeBit(int attributeIndex) {
        return 1L << attributeIndex;
    }

    public int getNumberOfVertices() { return numberOfVertices; }
    public long getMask() { return mask; }
    public PolygonVertex getVertex(int index) { return vertices[index]; }
}
