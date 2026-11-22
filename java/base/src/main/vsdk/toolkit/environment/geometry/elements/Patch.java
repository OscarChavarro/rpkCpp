package vsdk.toolkit.environment.geometry.elements;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateAxis;
import vsdk.toolkit.common.linealAlgebra.Jacobian;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.environment.geometry.element.PlanarFace;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.skin.BoundingBox;

public class Patch {

    public static final int MAXIMUM_VERTICES_PER_PATCH = 4;
    public static final int PATCH_VISIBILITY = 0x01;
    public static final int MAX_EXCLUDED_PATCHES = 4;

    // A static counter which is increased every time a Patch is created in
    // order to make a unique Patch id
    private static int patchId = 1;

    private static Vector3Df toFace(Vector3D v) {
        return new Vector3Df(v.x, v.y, v.z);
    }

    private static Vector3D fromFace(Vector3Df v) {
        return new Vector3D(v.x(), v.y(), v.z());
    }

    private int flags; // Other flags

    public int id; // Identification number for debugging, ID rendering
    private int faceIndex; // Dense zero-based index in the scene's face table, -1 until the scene assigns it
    public Patch twin; // Twin face (for double-sided surfaces)
    public Vertex[] vertex; // Pointers to the vertices
    public byte numberOfVertices; // Number of vertices: 3 or 4
    public BoundingBox boundingBox;
    // Geometry of the face: positions, plane, area, parametrization; immutable
    private PlanarFace face;
    public Vector3D normal; // face.getNormal(), in the type RPK's callers use
    public float planeConstant;
    public float tolerance; // Plane tolerance
    public float area; // Patch area
    public Vector3D midPoint; // Patch midpoint
    public Jacobian jacobian; // Shape-related constants for irregular quadrilaterals
    // Used for sampling the quadrilateral and for computing integrals
    public float directPotential; // Directly received hemispherical potential
    public CoordinateAxis index; // Indicates dominant part of patch normal
    public byte omit; // Indicates that the patch should not be considered
    // for a couple of things, such as intersection
    // testing, shaft culling, ... set to FALSE by
    // default. Do not forget to set to FALSE again
    // after you changed it.
    public ColorRgbMutable color; // Color used to flat render the patch
    // Data needed for radiance computations. Content depends on the current radiance algorithm / radiosity method (a.k.a. context)
    public Element radianceData;
    public RpkMaterial material;

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
    private static void pointInTriangle(
        Vector3D v0,
        Vector3D v1,
        Vector3D v2,
        float u,
        float v,
        Vector3D p) {
        p.x = v0.x + u * (v1.x - v0.x) + v * (v2.x - v0.x);
        p.y = v0.y + u * (v1.y - v0.y) + v * (v2.y - v0.y);
        p.z = v0.z + u * (v1.z - v0.z) + v * (v2.z - v0.z);
    }

    /**
    Point IN Quadrilateral: bi-linear parametrisation.
    */
    private static void pointInQuadrilateral(
        Vector3D v0,
        Vector3D v1,
        Vector3D v2,
        Vector3D v3,
        float u,
        float v,
        Vector3D p) {
        float c = u * v;
        float b = u - c;
        float d = v - c;
        float px = v0.x + b * (v1.x - v0.x);
        px = px + c * (v2.x - v0.x);
        px = px + d * (v3.x - v0.x);
        p.x = px;

        float py = v0.y + b * (v1.y - v0.y);
        py = py + c * (v2.y - v0.y);
        py = py + d * (v3.y - v0.y);
        p.y = py;

        float pz = v0.z + b * (v1.z - v0.z);
        pz = pz + c * (v2.z - v0.z);
        pz = pz + d * (v3.z - v0.z);
        p.z = pz;
    }

    private Vector3D getInterpolatedNormalAtUv(double u, double v) {
        Vector3D localNormal = new Vector3D();
        Vector3D v1 = vertex[0].normal;
        Vector3D v2 = vertex[1].normal;
        Vector3D v3 = vertex[2].normal;

        switch (numberOfVertices) {
            case 3:
                pointInTriangle(v1, v2, v3, (float)u, (float)v, localNormal);
                break;
            case 4:
                Vector3D v4 = vertex[3].normal;
                pointInQuadrilateral(v1, v2, v3, v4, (float)u, (float)v, localNormal);
                break;
            default:
                Logger.reportMessage(this, Logger.FATAL_ERROR, "PatchNormalAtUV", String.format("Invalid number of vertices %d", numberOfVertices));
                break;
        }

        localNormal.normalize(Numeric.EPSILON_FLOAT);
        return localNormal;
    }

    /**
    Adds the patch to the list of patches that share the vertex.
    */
    private void connectVertex(Vertex paramVertex) {
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
    public Patch(int inNumberOfVertices, Vertex v1, Vertex v2, Vertex v3, Vertex v4) {
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

        vertex = new Vertex[MAXIMUM_VERTICES_PER_PATCH];
        for (int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++) {
            vertex[i] = null;
        }

        numberOfVertices = (byte)inNumberOfVertices;
        vertex[0] = v1;
        vertex[1] = v2;
        vertex[2] = v3;
        vertex[3] = v4;

        // A bounding box will be computed the first time it is needed
        boundingBox = null;

        // Face geometry: normal, area and Jacobian, midpoint, plane, dominant axis
        final Vector3Df[] positions = new Vector3Df[] {
            toFace(v1.point),
            toFace(v2.point),
            toFace(v3.point),
            inNumberOfVertices == 4 ? toFace(v4.point) : new Vector3Df()
        };
        face = new PlanarFace(inNumberOfVertices, positions);
        if ( !face.isValid() ) {
            Logger.reportMessage("Patch", Logger.WARNING, "patchNormal", String.format("degenerate patch (id %d)", id));
            Statistics.instance().reader.numberOfElements--;
            Logger.reportMessage("Patch", Logger.ERROR, "Patch::Patch", "Error computing patch normal");
            System.exit(3);
        }
        normal = fromFace(face.getNormal());
        midPoint = fromFace(face.getMidPoint());
        jacobian = null;
        if ( face.hasJacobian() ) {
            jacobian = new Jacobian(face.getJacobianA(), face.getJacobianB(), face.getJacobianC());
        }
        area = face.getArea();
        planeConstant = face.getPlaneConstant();
        tolerance = face.getTolerance();
        index = CoordinateAxis.values()[face.getDominantAxis()];
        if ( face.getArea() < Numeric.EPSILON * Numeric.EPSILON ) {
            System.err.printf("Warning: very small patch id %d area = %g\n", id, face.getArea());
        }

        // Tell the vertices that there is a new Patch using them
        connectVertices();

        directPotential = 0.0f;
        color = new ColorRgbMutable();
        color.clear();

        omit = 0;
        flags = 0; // Other flags

        radianceData = null;
    }

    public int getFaceIndex() {
        return faceIndex;
    }

    public void setFaceIndex(int newFaceIndex) {
        faceIndex = newFaceIndex;
    }

    public void destroy() {
        jacobian = null;
        boundingBox = null;
        radianceData = null;
    }

    /**
    Computes a bounding box for the patch. fills it in getBoundingBox and returns
    a pointer to getBoundingBox.
    */
    public void computeAndGetBoundingBox(BoundingBox bounds) {
        computeBoundingBox();
        bounds.copyFrom(boundingBox);
    }

    public void computeBoundingBox() {
        if (boundingBox == null) {
            boundingBox = new BoundingBox();
            for (int i = 0; i < numberOfVertices; i++) {
                boundingBox.enlargeToIncludePoint(vertex[i].point);
            }
        }
    }

    /**
    Computes interpolated (= shading) normal at the point with given parameters
    on the patch.
    */
    private Vector3D interpolatedNormalAtUv(double u, double v) {
        if (!allVerticesHaveANormal()) {
            return normal;
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
    */
    public void interpolatedFrameAtUv(double u, double v, Vector3D X, Vector3D Y, Vector3D Z) {
        Z.copy(interpolatedNormalAtUv(u, v));

        if (X != null && Y != null) {
            double zz = Math.sqrt(1.0 - Z.z * Z.z);
            if (zz < Numeric.EPSILON) {
                X.set(1.0f, 0.0f, 0.0f);
            }
            else {
                X.set((float)(Z.y / zz), (float)(-Z.x / zz), 0.0f);
            }
            Y.crossProduct(Z, X); // Y = Z ^ X
        }
    }

    /**
    Returns texture coordinates determined from vertex texture coordinates and
    given u and v bi-linear of barycentric coordinates on the patch.
    */
    public Vector3D textureCoordAtUv(double u, double v) {
        Vector3D texCoord = new Vector3D();
        texCoord.set(0.0f, 0.0f, 0.0f);

        Vector3D t0 = vertex[0].textureCoordinates;
        Vector3D t1 = vertex[1].textureCoordinates;
        Vector3D t2 = vertex[2].textureCoordinates;

        switch (numberOfVertices) {
            case 3:
                if (t0 == null || t1 == null || t2 == null) {
                    texCoord.set((float)u, (float)v, 0.0f);
                }
                else {
                    pointInTriangle(t0, t1, t2, (float)u, (float)v, texCoord);
                }
                break;
            case 4:
                Vector3D t3 = vertex[3].textureCoordinates;
                if (t0 == null || t1 == null || t2 == null || t3 == null) {
                    texCoord.set((float)u, (float)v, 0.0f);
                }
                else {
                    pointInQuadrilateral(t0, t1, t2, t3, (float)u, (float)v, texCoord);
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
    public RayHit intersect(
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore,
        RayExclusionSet exclusions) {
        return intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions, sharedWorkspace);
    }

    /**
    Same as the method above, with the scratch of the calling thread. The hit
    record of the workspace mirrors the local stack variable of the C++ port:
    results are copied into the caller's hitStore before returning, so it never
    escapes this method (without hitStore the workspace's record is returned).
    */
    public RayHit intersect(
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore,
        RayExclusionSet exclusions,
        PatchIntersectionWorkspace workspace) {
        RayHit hit = workspace.hit;
        hit.setFlags(0);
        hit.setPatch(null);
        hit.setMaterial(null);

        if ( exclusions.excludesPatch(this) || exclusions.excludesFace(faceIndex) ) {
            return null;
        }

        // Side, plane hit, hit point and (u, v) in one call into the face
        final PlanarFace.RayHit faceHit = workspace.faceHit;
        final boolean inside = face.rayHit(
            toFace(ray.position),
            toFace(ray.direction),
            (hitFlags & RayHitFlag.FRONT) != 0,
            (hitFlags & RayHitFlag.BACK) != 0,
            minimumDistance,
            maximumDistance[0],
            faceHit);
        if ( inside ) {
            final float distance = faceHit.distance;
            Vector3D position = new Vector3D(faceHit.pointX, faceHit.pointY, faceHit.pointZ);
            hit.setPoint(position);
            hit.setUv(faceHit.u, faceHit.v);
            hit.setFlags(hit.getFlags()
                | (faceHit.side == PlanarFace.Side.BACK ? RayHitFlag.BACK : RayHitFlag.FRONT)
                | RayHitFlag.UV);
            hit.setPatch(this);
            hit.setMaterial(material);
            hit.setGeometricNormal(normal);
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
    public Vector3D pointBarycentricMapping(double u, double v, Vector3D point) {
        if ( hasZeroVertices() ) {
            return null;
        }

        point.copy(fromFace(face.pointAt(u, v)));
        return point;
    }

    /**
    Like above, except that always a uniform mapping is used (one that
    preserves area, with this mapping you will have more positions in stretched
    regions of an irregular quadrilateral, irregular quadrilaterals are the
    only ones for which this routine will yield other positions than the above
    routine).
    */
    public Vector3D uniformPoint(double u, double v, Vector3D point) {
        if ( hasZeroVertices() ) {
            return null;
        }
        point.copy(fromFace(face.uniformPointAt(u, v)));
        return point;
    }

    /**
    Computes (u, v) parameters of the point on the patch (barycentric or bi-linear
    parametrisation). Returns true if the point is inside the patch and false if
    not.
    WARNING: The (u, v) coordinates are correctly computed only for positions inside
    the patch. For positions outside, they can be garbage.
    */
    public boolean uv(Vector3D point, double[] u, double[] v) {
        final Vector2Dd uv = new Vector2Dd();
        final boolean inside = face.parameters(toFace(point), uv);
        u[0] = uv.x;
        v[0] = uv.y;
        return inside;
    }

    /**
    Computes a vertex color for the vertices of the patch.
    */
    public void computeVertexColors() {
        for (int i = 0; i < numberOfVertices; i++) {
            vertex[i].computeColor();
        }
    }

    /**
    Returns true if the two patches can see each other: P and Q see each
    other if at least a part of P is in front of Q and vice versa.
    */
    public boolean facing(Patch other) {
        return face.facing(other.face);
    }

    public static double clipToUnitInterval(double x) {
        if (x < Numeric.EPSILON) {
            return Numeric.EPSILON;
        }
        return x > (1.0 - Numeric.EPSILON) ? 1.0 - Numeric.EPSILON : x;
    }

    public void setVisible() {
        flags |= PATCH_VISIBILITY;
    }

    public void setInvisible() {
        flags &= ~PATCH_VISIBILITY;
    }

    public void setFlags(int newFlags) {
        flags = newFlags;
    }

    public int getFlags() {
        return flags;
    }

    /**
    Return true if patch is visible.
    */
    public boolean isVisible() {
        return (flags & PATCH_VISIBILITY) != 0;
    }

    /**
    Like uv, but returns uniform coordinates (inverse of uniformPoint()).
    */
    public boolean uniformUv(Vector3D point, double[] u, double[] v) {
        final Vector2Dd uv = new Vector2Dd();
        final boolean inside = face.uniformParameters(toFace(point), uv);
        u[0] = uv.x;
        v[0] = uv.y;
        return inside;
    }

    private int getNumberOfSamples() {
        int numberOfSamples = 1;
        if (material != null && material.getBsdf() != null && material.getBsdf().splitBsdfIsTextured()) {
            Vector3D t0 = vertex[0].textureCoordinates;
            Vector3D t1 = vertex[1].textureCoordinates;
            Vector3D t2 = vertex[2].textureCoordinates;
            Vector3D t3 = numberOfVertices == 3 ? t0 : vertex[3].textureCoordinates;

            boolean sameTex = t0 == t1 && t0 == t2 && t0 == t3 && t0 != null;
            numberOfSamples = sameTex ? 1 : 100;
        }
        return numberOfSamples;
    }
}
