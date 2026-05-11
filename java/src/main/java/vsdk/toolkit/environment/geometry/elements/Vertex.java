package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.skin.*;

import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;

public class Vertex {
    private static int currentComparisonFlags = VertexCompareFlags.VERTEX_COMPARE_LOCATION
        | VertexCompareFlags.VERTEX_COMPARE_NORMAL
        | VertexCompareFlags.VERTEX_COMPARE_TEXTURE_COORDINATE;

    public int id;
    public Vector3D point;
    public Vector3D normal;
    public Vector3D textureCoordinates;
    public ColorRgb color; // Used when rendering with Gouraud interpolation
    public ArrayList<Element> radianceData; // Data for the vertex maintained by the current radiance method
    public Vertex back; // Vertex at the same position, but with reversed normal, for back faces
    public ArrayList<Patch> patches; // List of references to patches sharing the vertex
    public int tmp; // Temporary (transient) storage for vertices used for saving VRML. Do not assume this value remains unchanged.

    /**
    Create a vertex with given coordinates, normal vector and list of patches
    sharing the vertex. Several vertices can share the same coordinates
    and normal vector. Several patches can share the same vertex.
    */
    public Vertex(
        Vector3D inPoint,
        Vector3D inNormal,
        Vector3D inTextureCoordinates,
        ArrayList<Patch> inPatches) {
        id = Statistics.instance().reader.numberOfVertices++;
        point = inPoint;
        normal = inNormal;
        textureCoordinates = inTextureCoordinates;
        patches = inPatches;
        color = new ColorRgb();
        color.set(0.0f, 0.0f, 0.0f);
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
    public void computeColor() {
        long numberOfPatches;

        color.set(0.0f, 0.0f, 0.0f);
        numberOfPatches = 0;

        if (patches != null) {
            for (int i = 0; i < patches.size(); i++) {
                Patch patch = patches.get(i);
                color.r += patch.color.r;
                color.g += patch.color.g;
                color.b += patch.color.b;
            }
            numberOfPatches = patches.size();
        }

        if (numberOfPatches > 0) {
            color.r /= (float)numberOfPatches;
            color.g /= (float)numberOfPatches;
            color.b /= (float)numberOfPatches;
        }
    }

    public static int setCompareFlags(int flags) {
        int oldFlags = currentComparisonFlags;
        currentComparisonFlags = flags;
        return oldFlags;
    }
}
