package vsdk.toolkit.scene;

import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.environment.geometry.elements.Patch;

/**
A structure describing polygons. Only used for shaft culling for the moment.

Note this is not able to represent a general polygon, just a convex polygon with
MAXIMUM_VERTICES_PER_PATCH or less (namely, triangles and quads only)
*/
public class Polygon {
    public Vector3D normal;
    public float planeConstant;
    public BoundingBox bounds;
    public Vector3D[] vertex;
    public int numberOfVertices;
    public byte index;

    public Polygon() {
        normal = new Vector3D();
        planeConstant = 0.0f;
        bounds = new BoundingBox();
        vertex = new Vector3D[Patch.MAXIMUM_VERTICES_PER_PATCH];
        for (int i = 0; i < vertex.length; i++) {
            vertex[i] = new Vector3D();
        }
        numberOfVertices = 0;
        index = 0;
    }
}
