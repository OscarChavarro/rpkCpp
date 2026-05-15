package vsdk.toolkit.render.sgl;

// Note: don't put > 32 doubles in Poly_vert, or mask will overflow
public class Polygon {
    public int n; // Number of sides
    public long mask; // Interpolation mask for vertex elems
    public PolygonVertex[] vertices;

    public Polygon() {
        vertices = new PolygonVertex[PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON];
        for (int i = 0; i < vertices.length; i++) {
            vertices[i] = new PolygonVertex();
        }
    }
}
