package vsdk.toolkit.skin;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.Material;

/**
Surfaces are basically a list of patches representing a simple object with given material.
*/
public final class MeshSurface extends Geometry {
    private static int nextSurfaceId = 0;

    /**
    Indicates on whether or not, and if so, which, colors are given when creating
    a new surface.
    */
    public static int colorFlags = MaterialColorFlags.NO_COLORS;

    public int meshId;
    public String objectName;

    /**
    The vertices of the patches. Each vertex contains a pointer to the vertex
    coordinates and normal vector at the vertex, which are in the MeshSurface positions
    and normals list. Different vertices can share the same coordinates and/or
    normals.
    */
    public ArrayList<Vertex> vertices;

    // A list of positions at the vertices of the patches of the surface
    public ArrayList<Vector3D> positions;

    // A list of normals at the vertices of the patches
    public ArrayList<Vector3D> normals;

    /**
    The patches making up the MeshSurface. Each patch contains pointers to three
    or four vertices in the vertices list of the MeshSurface. Different patches
    can share the same Vertex. Each vertex also contains a list of pointers to
    the patches that share the vertex. This can be used for e.g. Gouraud shading
    if a color is assigned to each vertex.
    */
    public ArrayList<Patch> faces;

    public Material material;

    /**
    This routine creates a MeshSurface with given material, positions.
    */
    public MeshSurface(
        String inObjectName,
        Material inMaterial,
        ArrayList<Vector3D> inPoints,
        ArrayList<Vector3D> inNormals,
        ArrayList<Vector3D> texCoords,
        ArrayList<Vertex> inVertices,
        ArrayList<Patch> inFaces,
        int inFlags) {
        super();

        Statistics.instance().reader.numberOfSurfaces++;

        id = nextGeometryId;
        nextGeometryId++;
        objectName = inObjectName;
        meshId = nextSurfaceId++;
        className = GeometryClassId.SURFACE_MESH;
        isDuplicate = false;

        material = inMaterial;
        positions = inPoints;
        normals = inNormals;
        vertices = inVertices;
        faces = inFaces;

        colorFlags = inFlags;

        // If colorFlags == VERTEX_COLORS, the inVertices are assumed to contain
        // the sum of the colors as used in each patch sharing the vertex
        if (colorFlags == MaterialColorFlags.VERTEX_COLORS) {
            for (int i = 0; vertices != null && i < vertices.size(); i++) {
                normalizeVertexColor(vertices.get(i));
            }
        }

        // Compute vertex colors
        if (colorFlags != MaterialColorFlags.VERTEX_COLORS) {
            for (int i = 0; vertices != null && i < vertices.size(); i++) {
                vertices.get(i).computeColor();
            }
        }

        colorFlags = MaterialColorFlags.NO_COLORS;

        patchListBounds(faces, boundingBox);

        // Enlarge bounding box a tiny bit for more conservative bounding box culling
        boundingBox.enlargeTinyBit();
        bounded = true;
        shaftCullGeometry = false;
        radianceData = null;
        itemCount = 0;
        omit = false;

        // texCoords is intentionally ignored in this C++ layer implementation.
    }

    @Override
    public void destroy() {
        objectName = null;

        if (positions != null) {
            positions.clear();
            positions = null;
        }

        if (normals != null) {
            normals.clear();
            normals = null;
        }

        if (vertices != null) {
            for (int i = 0; i < vertices.size(); i++) {
                vertices.get(i).destroy();
            }
            vertices.clear();
            vertices = null;
        }

        if (faces != null) {
            for (int i = 0; i < faces.size(); i++) {
                faces.get(i).destroy();
            }
            faces.clear();
            faces = null;
        }

        super.destroy();
    }

    private static void normalizeVertexColor(Vertex vertex) {
        long numberOfPatches = 0;

        if (vertex.patches != null) {
            numberOfPatches = vertex.patches.size();
        }

        if (numberOfPatches > 0) {
            vertex.color.r /= (float)numberOfPatches;
            vertex.color.g /= (float)numberOfPatches;
            vertex.color.b /= (float)numberOfPatches;
        }
    }

    /**
    DiscretizationIntersect returns null if the ray does not hit the discretization
    of the object. If the ray hits the object, a hit record is returned containing
    information about the intersection point. See geometry.h for more explanation.
    */
    @Override
    public RayHit discretizationIntersect(
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore) {
        return patchListIntersect(faces, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    }
}
