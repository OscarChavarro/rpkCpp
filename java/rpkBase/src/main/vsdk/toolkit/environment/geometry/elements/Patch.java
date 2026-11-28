package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.element.PlanarFace;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;

public class Patch {

    public static final int MAXIMUM_VERTICES_PER_PATCH = 4;

    // A static counter which is increased every time a Patch is created in
    // order to make a unique Patch id
    private static int patchId = 1;

    // Geometry of the face: positions, plane, area, parametrization; immutable
    private PlanarFace face;
    private RpkMaterial material;
    private int id; // Identification number for debugging, ID rendering
    private int faceIndex; // Dense zero-based index in the scene's face table, -1 until the scene assigns it
    private Patch twin; // Twin face (for double-sided surfaces)
    private final byte numberOfVertices; // Number of vertices: 3 or 4
    private final RpkVertex[] vertex; // Pointers to the vertices

    /**
    This routine returns the ID number the next patch would get.
    */
    public static int getNextId() {
        return patchId;
    }

    /**
    With this routine, the ID number is set that the next patch will get.
    Note that patch ID 0 is reserved. The smallest patch ID number should be 1.
    */
    public static void setNextId(int id) {
        patchId = id;
    }

    private boolean allVerticesHaveANormal() {
        int i;
        for (i = 0; i < numberOfVertices; i++) {
            if (vertex[i].normal == null) {
                break;
            }
        }
        return i >= numberOfVertices;
    }

    /**
    Point IN Triangle: barycentric parametrisation.
    */
    private static Vector3Dd pointInTriangle(
        Vector3Dd v0,
        Vector3Dd v1,
        Vector3Dd v2,
        double u,
        double v) {
        return new Vector3Dd(
            v0.x() + u * (v1.x() - v0.x()) + v * (v2.x() - v0.x()),
            v0.y() + u * (v1.y() - v0.y()) + v * (v2.y() - v0.y()),
            v0.z() + u * (v1.z() - v0.z()) + v * (v2.z() - v0.z()));
    }

    /**
    Point IN Quadrilateral: bi-linear parametrisation.
    */
    private static Vector3Dd pointInQuadrilateral(
        Vector3Dd v0,
        Vector3Dd v1,
        Vector3Dd v2,
        Vector3Dd v3,
        double u,
        double v) {
        double c = u * v;
        double b = u - c;
        double d = v - c;
        double px = v0.x() + b * (v1.x() - v0.x());
        px = px + c * (v2.x() - v0.x());
        px = px + d * (v3.x() - v0.x());

        double py = v0.y() + b * (v1.y() - v0.y());
        py = py + c * (v2.y() - v0.y());
        py = py + d * (v3.y() - v0.y());

        double pz = v0.z() + b * (v1.z() - v0.z());
        pz = pz + c * (v2.z() - v0.z());
        pz = pz + d * (v3.z() - v0.z());
        return new Vector3Dd(px, py, pz);
    }

    private Vector3Dd getInterpolatedNormalAtUv(double u, double v) {
        Vector3Dd localNormal = new Vector3Dd();
        Vector3Dd v1 = vertex[0].normal;
        Vector3Dd v2 = vertex[1].normal;
        Vector3Dd v3 = vertex[2].normal;

        switch (numberOfVertices) {
            case 3:
                localNormal = pointInTriangle(v1, v2, v3, (double)u, (double)v);
                break;
            case 4:
                Vector3Dd v4 = vertex[3].normal;
                localNormal = pointInQuadrilateral(v1, v2, v3, v4, (double)u, (double)v);
                break;
            default:
                Logger.reportMessage(this, Logger.FATAL_ERROR, "PatchNormalAtUV", String.format("Invalid number of vertices %d", numberOfVertices));
                break;
        }

        localNormal = localNormal.normalized(Numeric.EPSILON);
        return localNormal;
    }

    /**
    Adds the patch to the list of patches that share the vertex.
    */
    private void connectVertex(RpkVertex paramVertex) {
        if (paramVertex.patches == null) {
            return;
        }
        paramVertex.patches.add(this);
    }

    /**
    Adds the patch to the list of patches sharing each vertex.
    */
    private void connectVertices() {
        for (int i = 0; i < numberOfVertices; i++) {
            connectVertex(vertex[i]);
        }
    }

    /**
    Returns (u, v) coordinates of the point in the triangle.
    Didier Badouel, Graphics Gems I, p390.
    */
    // Scratch storage reused across calls on the hot ray intersection paths
    // (the C++ port keeps the equivalent variables on the stack) lives in a
    // PatchIntersectionWorkspace: each thread passes its own, and the single
    // threaded callers share this one
    private static final PatchIntersectionWorkspace sharedWorkspace = new PatchIntersectionWorkspace();

    /**
    Return true if patch is virtual.
    */
    public boolean hasZeroVertices() {
        return numberOfVertices == 0;
    }

    /**
    Creates a patch structure for a patch with given vertices.
    */
    public Patch(int inNumberOfVertices, RpkVertex v1, RpkVertex v2, RpkVertex v3, RpkVertex v4) {
        numberOfVertices = (byte)inNumberOfVertices;
        vertex = new RpkVertex[] {v1, v2, v3, v4};
        if (v1 == null || v2 == null || v3 == null || (inNumberOfVertices == 4 && v4 == null)) {
            Logger.reportMessage(this, Logger.ERROR, "Patch::Patch", "Null vertex");
            System.exit(1);
        }

        // It is sad but it is true
        if (inNumberOfVertices != 3 && inNumberOfVertices != 4) {
            Logger.reportMessage(this, Logger.ERROR, "Patch::Patch", "Can only handle quadrilateral or triangular patches");
            System.exit(2);
        }

        Statistics.instance().reader.numberOfElements++;
        twin = null;
        id = patchId;
        patchId++;
        faceIndex = -1;

        material = null;


        // Face geometry: normal, area and Jacobian, midpoint, plane, dominant axis
        final Vector3Dd[] positions = new Vector3Dd[] {
            v1.point,
            v2.point,
            v3.point,
            inNumberOfVertices == 4 ? v4.point : new Vector3Dd()
        };
        face = new PlanarFace(inNumberOfVertices, positions);
        if ( !face.isValid() ) {
            Logger.reportMessage("Patch", Logger.WARNING, "patchNormal", String.format("degenerate patch (id %d)", id));
            Statistics.instance().reader.numberOfElements--;
            Logger.reportMessage("Patch", Logger.ERROR, "Patch::Patch", "Error computing patch normal");
            System.exit(3);
        }
        if ( face.getArea() < VSDK.EPSILON * VSDK.EPSILON ) {
            System.err.printf("Warning: very small patch id %d area = %g\n", id, face.getArea());
        }

        // Tell the vertices that there is a new Patch using them
        connectVertices();
    }

    public double getArea() {
        return face.getArea();
    }

    public PlanarFace getFace() {
        return face;
    }

    public int getDominantAxisIndex() {
        return face.getDominantAxis();
    }

    public int getFaceIndex() {
        return faceIndex;
    }

    public void setFaceIndex(int newFaceIndex) {
        faceIndex = newFaceIndex;
    }

    public int getId() {
        return id;
    }

    public RpkMaterial getMaterial() {
        return material;
    }

    public Vector3Dd getNormal() {
        return face.getNormal();
    }

    public int getNumberOfVertices() {
        return numberOfVertices;
    }

    public double getPlaneConstant() {
        return face.getPlaneConstant();
    }

    public double getTolerance() {
        return face.getTolerance();
    }

    public Patch getTwin() {
        return twin;
    }

    public RpkVertex[] getVertices() {
        return vertex;
    }

    public Vector3Dd midPoint() {
        return face.getMidPoint();
    }

    public void setId(int newId) {
        id = newId;
    }

    public void setMaterial(RpkMaterial newMaterial) {
        material = newMaterial;
    }

    public void setTwin(Patch newTwin) {
        twin = newTwin;
    }

    /**
    Computes the patch's bounding box from its vertices, in vertex order. The
    vertices do not move after construction, so every call gives the same box;
    runtime queries read the scene's FaceBoundsTable, built once from this
    */
    public void computeBounds(RpkAxisAlignedBoundingBox bounds) {
        bounds.copyFrom(new RpkAxisAlignedBoundingBox());
        for ( int i = 0; i < numberOfVertices; i++ ) {
            bounds.enlargeToIncludePoint(vertex[i].point);
        }
    }

    /**
    Computes interpolated (= shading) normal at the point with given parameters
    on the patch.
    */
    private Vector3Dd interpolatedNormalAtUv(double u, double v) {
        if (!allVerticesHaveANormal()) {
            return face.getNormal();
        }
        return getInterpolatedNormalAtUv(u, v);
    }

    /**
    Computes shading frame at the given uv on the patch.
    Computes a interpolated (shading) frame at the uv or point with
    given parameters on the patch. The frame is consistent over the
    complete patch if the shading normals in the vertices do not differ
    too much from the geometric normal. The Z axis is the interpolated
    normal The X is determined by Z and the projection of the patch by
    the dominant axis (patch->index).
    X, Y and Z are one-element output holders; X and Y may be null.
    */
    public void interpolatedFrameAtUv(double u, double v, Vector3Dd[] X, Vector3Dd[] Y, Vector3Dd[] Z) {
        Z[0] = interpolatedNormalAtUv(u, v);

        if (X != null && Y != null) {
            double zz = Math.sqrt(1.0 - Z[0].z() * Z[0].z());
            if (zz < VSDK.EPSILON) {
                X[0] = new Vector3Dd(1.0, 0.0, 0.0);
            }
            else {
                X[0] = new Vector3Dd((double)(Z[0].y() / zz), (double)(-Z[0].x() / zz), 0.0);
            }
            Y[0] = Z[0].crossProduct(X[0]); // Y = Z ^ X
        }
    }

    /**
    Returns texture coordinates determined from vertex texture coordinates and
    given u and v bi-linear of barycentric coordinates on the patch.
    */
    public Vector3Dd textureCoordAtUv(double u, double v) {
        Vector3Dd texCoord = new Vector3Dd(0.0, 0.0, 0.0);

        Vector3Dd t0 = vertex[0].textureCoordinates;
        Vector3Dd t1 = vertex[1].textureCoordinates;
        Vector3Dd t2 = vertex[2].textureCoordinates;

        switch (numberOfVertices) {
            case 3:
                if (t0 == null || t1 == null || t2 == null) {
                    texCoord = new Vector3Dd((double)u, (double)v, 0.0);
                }
                else {
                    texCoord = pointInTriangle(t0, t1, t2, (double)u, (double)v);
                }
                break;
            case 4:
                Vector3Dd t3 = vertex[3].textureCoordinates;
                if (t0 == null || t1 == null || t2 == null || t3 == null) {
                    texCoord = new Vector3Dd((double)u, (double)v, 0.0);
                }
                else {
                    texCoord = pointInQuadrilateral(t0, t1, t2, t3, (double)u, (double)v);
                }
                break;
            default:
                Logger.reportMessage(this, Logger.FATAL_ERROR, "textureCoordAtUv", String.format("Invalid nr of vertices %d", numberOfVertices));
                break;
        }
        return texCoord;
    }

    /**
    Ray-patch intersection test, for computing form factors, creating ray-cast
    images ... Returns null if the Ray does not hit the patch. Fills in the
    hit record if there is a new hit and returns a pointer to it.
    Fills in the distance to the patch in maximumDistance if the patch
    is hit. Intersections closer than minimumDistance or further than *maximumDistance are
    ignored. The hitFlags determine what information to return about an
    intersection and whether front/back facing patches are to be
    considered and are described in ray.h.
    */
    public RpkRayHit intersect(
        Ray ray,
        double minimumDistance,
        double[] maximumDistance,
        int hitFlags,
        RpkRayHit hitStore,
        RayExclusionSet exclusions) {
        return intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions, sharedWorkspace);
    }

    /**
    Same as the method above, with the scratch of the calling thread. The hit
    record of the workspace mirrors the local stack variable of the C++ port:
    results are copied into the caller's hitStore before returning, so it never
    escapes this method (without hitStore the workspace's record is returned).
    */
    public RpkRayHit intersect(
        Ray ray,
        double minimumDistance,
        double[] maximumDistance,
        int hitFlags,
        RpkRayHit hitStore,
        RayExclusionSet exclusions,
        PatchIntersectionWorkspace workspace) {
        RpkRayHit hit = workspace.hit;
        hit.setFlags(0);
        hit.setPatch(null);
        hit.setMaterial(null);

        if ( exclusions.excludesPatch(this) || exclusions.excludesFace(faceIndex) ) {
            return null;
        }

        // Side, plane hit, hit point and (u, v) in one call into the face
        final PlanarFace.RayHit faceHit = workspace.faceHit;
        final boolean inside = face.rayHit(
            ray.getOrigin(),
            ray.getDirection(),
            (hitFlags & RayHitFlag.FRONT) != 0,
            (hitFlags & RayHitFlag.BACK) != 0,
            minimumDistance,
            maximumDistance[0],
            faceHit);
        if ( inside ) {
            final double distance = faceHit.distance;
            Vector3Dd position = new Vector3Dd(faceHit.pointX, faceHit.pointY, faceHit.pointZ);
            hit.setPoint(position);
            hit.setUv(faceHit.u, faceHit.v);
            hit.setFlags(hit.getFlags()
                | (faceHit.side == PlanarFace.Side.BACK ? RayHitFlag.BACK : RayHitFlag.FRONT)
                | RayHitFlag.UV);
            hit.setPatch(this);
            hit.setMaterial(material);
            hit.setGeometricNormal(face.getNormal());
            int newFlags = hit.getFlags()
                | RayHitFlag.PATCH
                | RayHitFlag.POINT
                | RayHitFlag.MATERIAL
                | RayHitFlag.GEOMETRIC_NORMAL
                | RayHitFlag.DISTANCE;
            hit.setFlags(newFlags);
            if ( (hitFlags & RayHitFlag.UV) != 0 && (hit.getFlags() & RayHitFlag.UV) == 0 ) {
                position = hit.getPoint();
                double[] u = workspace.u;
                double[] v = workspace.v;
                hit.getPatch().uv(position, u, v);
                hit.setUv(u[0], v[0]);
                hit.setPoint(position);
                newFlags = hit.getFlags() & RayHitFlag.UV;
                hit.setFlags(newFlags);
            }
            maximumDistance[0] = distance;

            if ( hitStore != null ) {
                hitStore.copyFrom(hit);
                return hitStore;
            }
            return hit;
        }

        return null;
    }

    /**
    Converts bi-linear coordinates into uniform (area preserving)
    coordinates for a polygon with given jacobian. Uniform coordinates
    are such that the area left of the line of positions with u-coordinate u,
    will be u.A and the area under the line of positions with v-coordinate v
    will be v.A.
    */
    public void biLinearToUniform(double[] u, double[] v) {
        final Vector2Dd uv = new Vector2Dd();
        uv.x = u[0];
        uv.y = v[0];
        face.biLinearToUniform(uv);
        u[0] = uv.x;
        v[0] = uv.y;
    }

    /**
    Converts uniform to bi-linear coordinates.
    */

    /**
    Given the parameter (u,v) of a point on the patch, this routine
    computes the 3D space coordinates of the same point, using barycentric
    mapping for a triangle and bi-linear mapping for a quadrilateral.
    u and v should be numbers in the range [0,1]. If u+v>1 for a triangle,
    (1-u) and (1-v) are used instead.
    */
    public Vector3Dd pointBarycentricMapping(double u, double v) {
        if ( hasZeroVertices() ) {
            return null;
        }

        return face.pointAt(u, v);
    }

    /**
    Like above, except that always a uniform mapping is used (one that
    preserves area, with this mapping you will have more positions in stretched
    regions of an irregular quadrilateral, irregular quadrilaterals are the
    only ones for which this routine will yield other positions than the above
    routine).
    */
    public Vector3Dd uniformPoint(double u, double v) {
        if ( hasZeroVertices() ) {
            return null;
        }
        return face.uniformPointAt(u, v);
    }

    /**
    Computes (u, v) parameters of the point on the patch (barycentric or bi-linear
    parametrisation). Returns true if the point is inside the patch and false if
    not.
    WARNING: The (u, v) coordinates are correctly computed only for positions inside
    the patch. For positions outside, they can be garbage.
    */
    public boolean uv(Vector3Dd point, double[] u, double[] v) {
        final Vector2Dd uv = new Vector2Dd();
        final boolean inside = face.parameters(point, uv);
        u[0] = uv.x;
        v[0] = uv.y;
        return inside;
    }

    /**
    Computes a vertex color for the vertices of the patch.
    */
    public void computeVertexColors(FaceColorTable faceColors) {
        for (int i = 0; i < numberOfVertices; i++) {
            vertex[i].computeColor(faceColors);
        }
    }

    /**
    Returns true if the two patches can see each other: P and Q see each
    other if at least a part of P is in front of Q and vice versa.
    */
    public boolean facing(Patch other) {
        return face.facing(other.face);
    }

    /**
    Like uv, but returns uniform coordinates (inverse of uniformPoint()).
    */
    public boolean uniformUv(Vector3Dd point, double[] u, double[] v) {
        final Vector2Dd uv = new Vector2Dd();
        final boolean inside = face.uniformParameters(point, uv);
        u[0] = uv.x;
        v[0] = uv.y;
        return inside;
    }
}
