package vsdk.toolkit.scene;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.environment.geometry.elements.Patch;

/**
A structure describing polygons. Only used for shaft culling for the moment.

Note this is not able to represent a general polygon, just a convex polygon with
MAXIMUM_VERTICES_PER_PATCH or less (namely, triangles and quads only)
*/
public class ConvexPolygon {
    public Vector3Dd normal;
    public double planeConstant;
    public RpkAxisAlignedBoundingBox bounds;
    public Vector3Dd[] vertex;
    public int numberOfVertices;
    public byte index;

    public ConvexPolygon() {
        normal = new Vector3Dd();
        planeConstant = 0.0;
        bounds = new RpkAxisAlignedBoundingBox();
        vertex = new Vector3Dd[Patch.MAXIMUM_VERTICES_PER_PATCH];
        for (int i = 0; i < vertex.length; i++) {
            vertex[i] = new Vector3Dd();
        }
        numberOfVertices = 0;
        index = 0;
    }
}
