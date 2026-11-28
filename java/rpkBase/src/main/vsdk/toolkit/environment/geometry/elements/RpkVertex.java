package vsdk.toolkit.environment.geometry.elements;

import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.Statistics;

public class RpkVertex {
    private static int currentComparisonFlags = VertexCompareFlags.VERTEX_COMPARE_LOCATION
        | VertexCompareFlags.VERTEX_COMPARE_NORMAL
        | VertexCompareFlags.VERTEX_COMPARE_TEXTURE_COORDINATE;

    public int id;
    public Vector3Dd point;
    public Vector3Dd normal;
    public Vector3Dd textureCoordinates;
    public ColorRgb color; // Used when rendering with Gouraud interpolation
    public ArrayList<Element> radianceData; // Data for the vertex maintained by the current radiance method
    public RpkVertex back; // Vertex at the same position, but with reversed normal, for back faces
    public ArrayList<Patch> patches; // List of references to patches sharing the vertex
    public int tmp; // Temporary (transient) storage for vertices used for saving VRML. Do not assume this value remains unchanged.

    /**
    Create a vertex with given coordinates, normal vector and list of patches
    sharing the vertex. Several vertices can share the same coordinates
    and normal vector. Several patches can share the same vertex.
    */
    public RpkVertex(
        Vector3Dd inPoint,
        Vector3Dd inNormal,
        Vector3Dd inTextureCoordinates,
        ArrayList<Patch> inPatches) {
        id = Statistics.instance().reader.numberOfVertices++;
        point = inPoint;
        normal = inNormal;
        textureCoordinates = inTextureCoordinates;
        patches = inPatches;
        color = new ColorRgb();
        radianceData = null;
        back = null;
        tmp = 0;
    }

    /**
    Destroys the vertex. Does not destroy the coordinate vector and
    normal vector, neither the patches sharing the vertex.
    */
    public void destroy() {
        Statistics.instance().reader.numberOfVertices--;
        patches = null;
    }

    /**
    Averages the color of each patch sharing the vertex and assigns the
    resulting color to the vertex.
    */
    public void computeColor(FaceColorTable faceColors) {
        long numberOfPatches;

        color = new ColorRgb(0.0, 0.0, 0.0);
        numberOfPatches = 0;

        if (patches != null) {
            double r = 0.0;
            double g = 0.0;
            double b = 0.0;
            for (int i = 0; i < patches.size(); i++) {
                // Before the scene's face table exists (loading) every face is black
                if ( faceColors != null ) {
                    final ColorRgb patchColor = faceColors.get(patches.get(i).getFaceIndex());
                    r += patchColor.getR();
                    g += patchColor.getG();
                    b += patchColor.getB();
                }
            }
            color = new ColorRgb(r, g, b);
            numberOfPatches = patches.size();
        }

        if (numberOfPatches > 0) {
            final double invPatchCount = 1.0 / (double)numberOfPatches;
            color = new ColorRgb(color.getR() * invPatchCount, color.getG() * invPatchCount, color.getB() * invPatchCount);
        }
    }

    public static int setCompareFlags(int flags) {
        int oldFlags = currentComparisonFlags;
        currentComparisonFlags = flags;
        return oldFlags;
    }
}
