package vsdk.toolkit.skin;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.MaterialColorFlags;

import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;

import java.util.ArrayList;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.RpkMaterial;

/**
Surfaces are basically a list of patches representing a simple object with given material.
*/
public final class MeshSurface extends RpkGeometry {
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
    public ArrayList<RpkVertex> vertices;

    // A list of positions at the vertices of the patches of the surface
    public ArrayList<Vector3Dd> positions;

    // A list of normals at the vertices of the patches
    public ArrayList<Vector3Dd> normals;

    /**
    The patches making up the MeshSurface. Each patch contains pointers to three
    or four vertices in the vertices list of the MeshSurface. Different patches
    can share the same Vertex. Each vertex also contains a list of pointers to
    the patches that share the vertex. This can be used for e.getG(). Gouraud shading
    if a color is assigned to each vertex.
    */
    public ArrayList<Patch> faces;

    public RpkMaterial material;

    /**
    This routine creates a MeshSurface with given material, positions.
    */
    public MeshSurface(
        String inObjectName,
        RpkMaterial inMaterial,
        ArrayList<Vector3Dd> inPoints,
        ArrayList<Vector3Dd> inNormals,
        ArrayList<Vector3Dd> texCoords,
        ArrayList<RpkVertex> inVertices,
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
                vertices.get(i).computeColor(null); // No face colors exist while loading
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
            faces.clear();
            faces = null;
        }

        super.destroy();
    }

    private static void normalizeVertexColor(RpkVertex vertex) {
        long numberOfPatches = 0;

        if (vertex.patches != null) {
            numberOfPatches = vertex.patches.size();
        }

        if (numberOfPatches > 0) {
            final double invPatchCount = 1.0 / (double)numberOfPatches;
            vertex.color = new ColorRgb(
                vertex.color.getR() * invPatchCount,
                vertex.color.getG() * invPatchCount,
                vertex.color.getB() * invPatchCount);
        }
    }

    /**
    DiscretizationIntersect returns null if the ray does not hit the discretization
    of the object. If the ray hits the object, a hit record is returned containing
    information about the intersection point. See geometry.h for more explanation.
    */
    @Override
    public RpkRayHit discretizationIntersect(
        Ray ray,
        double minimumDistance,
        double[] maximumDistance,
        int hitFlags,
        RpkRayHit hitStore,
        RayExclusionSet exclusions) {
        return patchListIntersect(faces, ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
    }
}
