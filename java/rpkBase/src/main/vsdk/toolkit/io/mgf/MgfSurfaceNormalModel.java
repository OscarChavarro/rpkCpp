package vsdk.toolkit.io.mgf;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

/**
Vertex normals of the patches of a MGF primitive, in the local frame of the
VITRAL primitive that tessellates it, and the choice of the faces of its
boundary representation that become patches.

The normals are the ones the classic MGF tessellation gave (the one of the
MGF parser library, where spheres and tori were expanded into cones whose
end normals were "warped"): the exact normal of the curved surface at each
vertex for spheres and tori, the normal of the cone for cones and cylinders
(constant along each generatrix, and the axis at the apex), and none for
planar primitives.

Open primitives (cones, cylinders and rings) are exported by VITRAL as
laminas: two coincident sheets of faces with opposite orientations. Only
the faces of the sheet oriented as the MGF surface become patches.

Immutable value.
*/
public final class MgfSurfaceNormalModel {
    public enum Kind {
        /** Closed planar solid: every face, no vertex normals */
        FLAT_SOLID,
        /** Lamina on the plane z = 0: the faces looking to +Z, no normals */
        FLAT_LAMINA,
        /** Sphere centered at the origin: every face, radial normals */
        SPHERE,
        /** Lamina of a cone (or cylinder) along +Z, from radius
        `bottomRadius` at z = 0 to radius `topRadius` at z = `height`:
        the faces looking away from the axis */
        CONE_LAMINA,
        /** Torus around the Z axis, of major radius `majorRadius` */
        TORUS
    }

    private final Kind kind;
    private final double bottomRadius;
    private final double topRadius;
    private final double height;
    private final double majorRadius;

    private MgfSurfaceNormalModel(Kind kind, double bottomRadius, double topRadius, double height, double majorRadius) {
        this.kind = kind;
        this.bottomRadius = bottomRadius;
        this.topRadius = topRadius;
        this.height = height;
        this.majorRadius = majorRadius;
    }

    public static MgfSurfaceNormalModel flatSolid() {
        return new MgfSurfaceNormalModel(Kind.FLAT_SOLID, 0.0, 0.0, 0.0, 0.0);
    }

    public static MgfSurfaceNormalModel flatLamina() {
        return new MgfSurfaceNormalModel(Kind.FLAT_LAMINA, 0.0, 0.0, 0.0, 0.0);
    }

    public static MgfSurfaceNormalModel sphere() {
        return new MgfSurfaceNormalModel(Kind.SPHERE, 0.0, 0.0, 0.0, 0.0);
    }

    public static MgfSurfaceNormalModel coneLamina(double bottomRadius, double topRadius, double height) {
        return new MgfSurfaceNormalModel(Kind.CONE_LAMINA, bottomRadius, topRadius, height, 0.0);
    }

    public static MgfSurfaceNormalModel torus(double majorRadius) {
        return new MgfSurfaceNormalModel(Kind.TORUS, 0.0, 0.0, 0.0, majorRadius);
    }

    public Kind getKind() {
        return kind;
    }

    /**
    @return true if the patches get vertex normals
    */
    public boolean hasVertexNormals() {
        return kind == Kind.SPHERE || kind == Kind.CONE_LAMINA || kind == Kind.TORUS;
    }

    /**
    @param localPoint a vertex of the tessellation, in the local frame
    @return the (outwards, not necessarily unit) normal of the surface at the
    vertex, in the local frame
    */
    public Vector3Dd vertexNormal(Vector3Dd localPoint) {
        double radial = Math.sqrt(localPoint.x() * localPoint.x() + localPoint.y() * localPoint.y());

        switch ( kind ) {
            case SPHERE:
                return localPoint;
            case CONE_LAMINA:
                if ( radial <= VSDK.EPSILON * (bottomRadius + topRadius) ) {
                    // The apex: the classic tessellation gives it the axis
                    return new Vector3Dd(0.0, 0.0, 1.0);
                }
                // Radial direction, raised by the slope of the generatrix
                return new Vector3Dd(localPoint.x() / radial, localPoint.y() / radial, (bottomRadius - topRadius) / height);
            case TORUS:
                if ( radial <= 0.0 ) {
                    return new Vector3Dd(0.0, 0.0, 0.0);
                }
                // From the closest point of the core circle
                return new Vector3Dd(
                    localPoint.x() - majorRadius * localPoint.x() / radial,
                    localPoint.y() - majorRadius * localPoint.y() / radial,
                    localPoint.z());
            default:
                return new Vector3Dd(0.0, 0.0, 0.0);
        }
    }

    /**
    @param localCentroid centroid of a face of the boundary representation
    @param localFaceNormal normal of the face (following its loop)
    @return true if the face becomes a patch
    */
    public boolean selectsFace(Vector3Dd localCentroid, Vector3Dd localFaceNormal) {
        switch ( kind ) {
            case FLAT_LAMINA:
                return localFaceNormal.z() > 0.0;
            case CONE_LAMINA:
                return localFaceNormal.dotProduct(vertexNormal(localCentroid)) > 0.0;
            default:
                return true;
        }
    }
}
