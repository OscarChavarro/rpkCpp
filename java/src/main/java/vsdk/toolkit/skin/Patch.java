package vsdk.toolkit.skin;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.CoordinateAxis;
import vsdk.toolkit.common.linealAlgebra.Jacobian;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.material.RayHitFlag;

public class Patch {
    public static final int MAXIMUM_VERTICES_PER_PATCH = 4;
    public static final int PATCH_VISIBILITY = 0x01;
    public static final int MAX_EXCLUDED_PATCHES = 4;

    private static final double TOLERANCE = 1e-5;

    // A static counter which is increased every time a Patch is created in
    // order to make a unique Patch id
    private static int patchId = 1;
    private static final Patch[] excludedPatches = new Patch[] {null, null, null, null};

    private int flags; // Other flags

    public int id; // Identification number for debugging, ID rendering
    public Patch twin; // Twin face (for double-sided surfaces)
    public Vertex[] vertex; // Pointers to the vertices
    public byte numberOfVertices; // Number of vertices: 3 or 4
    public BoundingBox boundingBox;
    public Vector3D normal;
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
    public ColorRgb color; // Color used to flat render the patch
    // Data needed for radiance computations. Content depends on the current radiance algorithm / radiosity method (a.k.a. context)
    public Element radianceData;
    public Material material;

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

    private boolean isExcluded() {
        // MAX_EXCLUDED_PATCHES tests
        for (int i = 0; i < MAX_EXCLUDED_PATCHES; i++) {
            if (excludedPatches[i] == this) {
                return true;
            }
        }
        return false;
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
        p.x = Math.fma(v, (v2.x - v0.x), Math.fma(u, (v1.x - v0.x), v0.x));
        p.y = Math.fma(v, (v2.y - v0.y), Math.fma(u, (v1.y - v0.y), v0.y));
        p.z = Math.fma(v, (v2.z - v0.z), Math.fma(u, (v1.z - v0.z), v0.z));
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
        p.x = Math.fma(d, (v3.x - v0.x), Math.fma(c, (v2.x - v0.x), Math.fma(b, (v1.x - v0.x), v0.x)));
        p.y = Math.fma(d, (v3.y - v0.y), Math.fma(c, (v2.y - v0.y), Math.fma(b, (v1.y - v0.y), v0.y)));
        p.z = Math.fma(d, (v3.z - v0.z), Math.fma(c, (v2.z - v0.z), Math.fma(b, (v1.z - v0.z), v0.z)));
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
                Error.fatal(-1, "PatchNormalAtUV", "Invalid number of vertices %d", numberOfVertices);
                break;
        }

        localNormal.normalize(Numeric.EPSILON_FLOAT);
        return localNormal;
    }

    /**
    Looks for solution of the quadratic equation A.u^2 + B.u + C = 0
    in the interval [0,1]. There must be exactly one such solution.
    Returns true if one such solution is found, or false if the equation
    has no real solutions, both solutions are in the interval [0,1] or
    none of them is. In case of problems, best guess solution
    is returned. Problems seem to be due to numerical inaccuracy.
    */
    private static boolean solveQuadraticUnitInterval(double A, double B, double C, double[] x) {
        double D = B * B - 4.0 * A * C;
        double x1;
        double x2;

        if (A < TOLERANCE && A > -TOLERANCE) {
            // Degenerate case, solve B*x + C = 0
            x1 = -1.0;
            x2 = -C / B;
        }
        else {
            if (D < -TOLERANCE * TOLERANCE) {
                x[0] = -B / (2.0 * A);
                Error.error(
                    null,
                    "Bi-linear->Uniform mapping has negative discriminant D = %g. Taking 0 as discriminant and %g as solution.",
                    D,
                    x[0]);
                return false;
            }

            D = D > TOLERANCE * TOLERANCE ? Math.sqrt(D) : 0.0;
            A = 1.0 / (2.0 * A);
            x1 = (-B + D) * A;
            x2 = (-B - D) * A;

            if (x1 > -TOLERANCE && x1 < 1.0 + TOLERANCE) {
                x[0] = x1;
                if (x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE) {
                    // Error: Bi-linear->Uniform mapping ambiguous: x1, x2, taking x1 as solution
                    return false;
                }
                return true;
            }
        }

        if (x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE) {
            x[0] = x2;
            return true;
        }

        // May happen due to numerical errors choose the root closest to [0,1]
        double d;
        if (x1 > 1.0) {
            d = x1 - 1.0;
        }
        else {
            // x1 < 0.0
            d = -x1;
        }
        x[0] = x1;
        if (x2 > 1.0) {
            if (x2 - 1.0 < d) {
                x[0] = x2;
            }
        }
        else if (0.0 - x2 < d) {
            x[0] = x2;
        }

        // Clip it to [0,1]
        if (x[0] < 0.0) {
            x[0] = 0.0;
        }
        if (x[0] > 1.0) {
            x[0] = 1.0;
        }
        return false;
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
    Compute the jacobian J(u, v) of the coordinate mapping from the unit
    square [0, 1] ^ 2 or the standard triangle (0, 0), (1, 0), (0, 1) to the patch.
    - For triangles: J(u, v) = the area of the triangle.
    - For quadrilaterals: J(u, v) = A + B.u + C.v
      the area of the patch = A + (B + C) / 2
      for parallelograms holds that B = C = 0
      the coefficients A, B, C are only stored if B and C are non-zero.
    The normal of the patch should have been computed before calling this routine.
    */
    private float computeRandomWalkRadiosityArea() {
        Vector3D p1;
        Vector3D p2;
        Vector3D p3;
        Vector3D p4;
        Vector3D d1 = new Vector3D();
        Vector3D d2 = new Vector3D();
        Vector3D d3 = new Vector3D();
        Vector3D d4 = new Vector3D();
        Vector3D cp1 = new Vector3D();
        Vector3D cp2 = new Vector3D();
        Vector3D cp3 = new Vector3D();
        float a;
        float b;
        float c;

        // Explicitly compute the area and jacobian
        switch (numberOfVertices) {
            case 3:
                // Jacobian J(u,v) for the mapping of the triangle
                // (0,0),(0,1),(1,0) to a triangular patch is constant and equal
                // to the area of the triangle ... so there is no need to store
                // any coefficients explicitly
                jacobian = null;

                p1 = vertex[0].point;
                p2 = vertex[1].point;
                p3 = vertex[2].point;
                d1.subtraction(p2, p1);
                d2.subtraction(p3, p2);
                cp1.crossProduct(d1, d2);
                area = 0.5f * cp1.norm();
                break;
            case 4:
                p1 = vertex[0].point;
                p2 = vertex[1].point;
                p3 = vertex[2].point;
                p4 = vertex[3].point;
                d1.subtraction(p2, p1);
                d2.subtraction(p3, p2);
                d3.subtraction(p3, p4);
                d4.subtraction(p4, p1);
                cp1.crossProduct(d1, d4);
                cp2.crossProduct(d1, d3);
                cp3.crossProduct(d2, d4);
                a = cp1.dotProduct(normal);
                b = cp2.dotProduct(normal);
                c = cp3.dotProduct(normal);

                area = a + 0.5f * (b + c);
                if (area < 0.0f) {
                    // May happen if the normal direction and
                    // vertex order are not consistent
                    b = -b;
                    c = -c;
                    area = -area;
                }

                // b and c are zero for parallelograms. In that case, the area is equal to
                // a, so we do not need to store the coefficients
                if (Math.abs(b) / area < Numeric.EPSILON && Math.abs(c) / area < Numeric.EPSILON) {
                    jacobian = null;
                }
                else {
                    jacobian = new Jacobian(a, b, c);
                }
                break;
            default:
                Error.fatal(2, "computeRandomWalkRadiosityArea", "Can only handle triangular and quadrilateral patches.");
                jacobian = null;
                area = 0.0f;
                break;
        }

        if (area < Numeric.EPSILON * Numeric.EPSILON) {
            System.err.printf("Warning: very small patch id %d area = %g%n", id, area);
        }

        return area;
    }

    /**
    Computes the mid point of the patch, stores the result in p and
    returns a pointer to p.
    */
    private void computeMidpoint(Vector3D p) {
        p.set(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < numberOfVertices; i++) {
            p.addition(p, vertex[i].point);
        }
        p.inverseScaledCopy(numberOfVertices, p, Numeric.EPSILON_FLOAT);
    }

    /**
    Computes a certain width for the plane, e.g. for co-planar testing.
    */
    private float computeTolerance() {
        // Fill in the vertices in the plane equation + take into account the vertex position tolerance
        float localTolerance = 0.0f;
        for (int i = 0; i < numberOfVertices; i++) {
            Vector3D p = vertex[i].point;
            float e = Math.abs(normal.dotProduct(p) + planeConstant) + p.tolerance(Numeric.EPSILON_FLOAT);
            if (e > localTolerance) {
                localTolerance = e;
            }
        }
        return localTolerance;
    }

    /**
    Returns (u, v) coordinates of the point in the triangle.
    Didier Badouel, Graphics Gems I, p390.
    */
    private boolean triangleUv(Vector3D point, Vector2Dd uv) {
        double u0;
        double v0;
        double alpha;
        double beta;
        Vector2Dd p0 = new Vector2Dd();
        Vector2Dd p1 = new Vector2Dd();
        Vector2Dd p2 = new Vector2Dd();

        // Project to 2D
        int vertexIndex = 0;
        switch (index) {
            case X:
                u0 = vertex[vertexIndex].point.y;
                v0 = vertex[vertexIndex].point.z;
                Vector2Dd.set(p0, point.y - u0, point.z - v0);
                vertexIndex++;
                Vector2Dd.set(p1, vertex[vertexIndex].point.y - u0, vertex[vertexIndex].point.z - v0);
                vertexIndex++;
                Vector2Dd.set(p2, vertex[vertexIndex].point.y - u0, vertex[vertexIndex].point.z - v0);
                break;

            case Y:
                u0 = vertex[vertexIndex].point.x;
                v0 = vertex[vertexIndex].point.z;
                Vector2Dd.set(p0, point.x - u0, point.z - v0);
                vertexIndex++;
                Vector2Dd.set(p1, vertex[vertexIndex].point.x - u0, vertex[vertexIndex].point.z - v0);
                vertexIndex++;
                Vector2Dd.set(p2, vertex[vertexIndex].point.x - u0, vertex[vertexIndex].point.z - v0);
                break;

            case Z:
                u0 = vertex[vertexIndex].point.x;
                v0 = vertex[vertexIndex].point.y;
                Vector2Dd.set(p0, point.x - u0, point.y - v0);
                vertexIndex++;
                Vector2Dd.set(p1, vertex[vertexIndex].point.x - u0, vertex[vertexIndex].point.y - v0);
                vertexIndex++;
                Vector2Dd.set(p2, vertex[vertexIndex].point.x - u0, vertex[vertexIndex].point.y - v0);
                break;

            default:
                break;
        }

        if (p1.u < -Numeric.EPSILON || p1.u > Numeric.EPSILON) {
            // p1.u non zero
            beta = (p0.v * p1.u - p0.u * p1.v) / (p2.v * p1.u - p2.u * p1.v);
            if (beta >= 0.0 && beta <= 1.0) {
                alpha = (p0.u - beta * p2.u) / p1.u;
            }
            else {
                return false;
            }
        }
        else {
            beta = p0.u / p2.u;
            if (beta >= 0.0 && beta <= 1.0) {
                alpha = (p0.v - beta * p2.v) / p1.v;
            }
            else {
                return false;
            }
        }

        uv.u = alpha;
        uv.v = beta;
        return !(alpha < 0.0 || (alpha + beta) > 1.0);
    }

    /**
    Christophe Schlick and Gilles Subrenat (15 May 1994)
    Ray Intersection of Tessellated Surfaces: Quadrangles versus Triangles
    in Graphics Gems V (edited by A. Paeth), Academic Press, pages 232-241.
    */
    private static boolean quadUv(Patch patch, Vector3D point, Vector2Dd uv) {
        Vector2Dd A = new Vector2Dd(); // Projected vertices
        Vector2Dd B = new Vector2Dd();
        Vector2Dd C = new Vector2Dd();
        Vector2Dd D = new Vector2Dd();
        Vector2Dd M = new Vector2Dd(); // Projected intersection point
        Vector2Dd AB = new Vector2Dd(); // AE = DC - AB = DA - CB
        Vector2Dd BC = new Vector2Dd();
        Vector2Dd CD = new Vector2Dd();
        Vector2Dd AD = new Vector2Dd();
        Vector2Dd AM = new Vector2Dd();
        Vector2Dd AE = new Vector2Dd();
        double u = -1.0; // Parametric coordinates
        double v = -1.0;
        Vector2Dd vector = new Vector2Dd(); // Temporary 2D-vector
        boolean isInside = false;

        // Projection on the plane that is most parallel to the facet
        int vertexIndex = 0;
        switch (patch.index) {
            case X:
                Vector2Dd.set(A, patch.vertex[vertexIndex].point.y, patch.vertex[vertexIndex].point.z);
                vertexIndex++;
                Vector2Dd.set(B, patch.vertex[vertexIndex].point.y, patch.vertex[vertexIndex].point.z);
                vertexIndex++;
                Vector2Dd.set(C, patch.vertex[vertexIndex].point.y, patch.vertex[vertexIndex].point.z);
                vertexIndex++;
                Vector2Dd.set(D, patch.vertex[vertexIndex].point.y, patch.vertex[vertexIndex].point.z);
                Vector2Dd.set(M, point.y, point.z);
                break;

            case Y:
                Vector2Dd.set(A, patch.vertex[vertexIndex].point.x, patch.vertex[vertexIndex].point.z);
                vertexIndex++;
                Vector2Dd.set(B, patch.vertex[vertexIndex].point.x, patch.vertex[vertexIndex].point.z);
                vertexIndex++;
                Vector2Dd.set(C, patch.vertex[vertexIndex].point.x, patch.vertex[vertexIndex].point.z);
                vertexIndex++;
                Vector2Dd.set(D, patch.vertex[vertexIndex].point.x, patch.vertex[vertexIndex].point.z);
                Vector2Dd.set(M, point.x, point.z);
                break;

            case Z:
                Vector2Dd.set(A, patch.vertex[vertexIndex].point.x, patch.vertex[vertexIndex].point.y);
                vertexIndex++;
                Vector2Dd.set(B, patch.vertex[vertexIndex].point.x, patch.vertex[vertexIndex].point.y);
                vertexIndex++;
                Vector2Dd.set(C, patch.vertex[vertexIndex].point.x, patch.vertex[vertexIndex].point.y);
                vertexIndex++;
                Vector2Dd.set(D, patch.vertex[vertexIndex].point.x, patch.vertex[vertexIndex].point.y);
                Vector2Dd.set(M, point.x, point.y);
                break;

            default:
                break;
        }

        Vector2Dd.subtract(B, A, AB);
        Vector2Dd.subtract(C, B, BC);
        Vector2Dd.subtract(D, C, CD);
        Vector2Dd.subtract(D, A, AD);
        Vector2Dd.add(CD, AB, AE);
        Vector2Dd.negate(AE);
        Vector2Dd.subtract(M, A, AM);

        double a; // Quadratic equation
        double b;
        double c;

        if (Math.abs(Vector2Dd.determinant(AB, CD)) < Numeric.EPSILON) {
            // Case AB // CD
            Vector2Dd.subtract(AB, CD, vector);
            v = Vector2Dd.determinant(AM, vector) / Vector2Dd.determinant(AD, vector);
            if (v >= 0.0 && v <= 1.0) {
                b = Vector2Dd.determinant(AB, AD) - Vector2Dd.determinant(AM, AE);
                c = Vector2Dd.determinant(AM, AD);
                u = Math.abs(b) < Numeric.EPSILON ? -1.0 : c / b;
                isInside = (u >= 0.0 && u <= 1.0);
            }
        }
        else if (Math.abs(Vector2Dd.determinant(BC, AD)) < Numeric.EPSILON) {
            // Case AD // BC
            Vector2Dd.add(AD, BC, vector);
            u = Vector2Dd.determinant(AM, vector) / Vector2Dd.determinant(AB, vector);
            if (u >= 0.0 && u <= 1.0) {
                b = Vector2Dd.determinant(AD, AB) - Vector2Dd.determinant(AM, AE);
                c = Vector2Dd.determinant(AM, AB);
                v = Math.abs(b) < Numeric.EPSILON ? -1.0 : c / b;
                isInside = (v >= 0.0 && v <= 1.0);
            }
        }
        else {
            // General case
            a = Vector2Dd.determinant(AB, AE);
            c = -Vector2Dd.determinant(AM, AD);
            b = Vector2Dd.determinant(AB, AD) - Vector2Dd.determinant(AM, AE);
            a = -0.5 / a;
            b *= a;
            c *= (a + a);
            double sqrtDelta = b * b + c;
            if (sqrtDelta >= 0.0) {
                sqrtDelta = Math.sqrt(sqrtDelta);
                u = b - sqrtDelta;
                if (u < 0.0 || u > 1.0) {
                    // To choose u between 0 and 1
                    u = b + sqrtDelta;
                }
                if (u >= 0.0 && u <= 1.0) {
                    v = AD.u + u * AE.u;
                    if (Math.abs(v) < Numeric.EPSILON) {
                        v = (AM.v - u * AB.v) / (AD.v + u * AE.v);
                    }
                    else {
                        v = (AM.u - u * AB.u) / v;
                    }
                    isInside = (v >= 0.0 && v <= 1.0);
                }
            }
            else {
                u = -1.0;
                v = -1.0;
            }
        }

        uv.u = clipToUnitInterval(u);
        uv.v = clipToUnitInterval(v);
        return isInside;
    }

    /**
    Badouels and Schlicks method from graphics gems: slower, but consumes less storage and computes
    (u,v) parameters as a side result.
    */
    private boolean hitInPatch(RayHit hit, Patch patch) {
        int newFlags = hit.getFlags() | RayHitFlag.UV;
        hit.setFlags(newFlags); // uv parameters computed as a side result
        Vector3D position = hit.getPoint();
        Vector2Dd uv = new Vector2Dd();
        boolean result = (patch.numberOfVertices == 3)
            ? triangleUv(position, uv)
            : quadUv(patch, position, uv);
        hit.setUv(uv);
        return result;
    }

    /**
    Returns a pointer to the normal vector if everything is OK. Null pointer if degenerate polygon.
    */
    private static Vector3D patchNormal(Patch patch, Vector3D normal) {
        Vector3D current = new Vector3D();

        normal.set(0.0f, 0.0f, 0.0f);
        current.subtraction(patch.vertex[patch.numberOfVertices - 1].point, patch.vertex[0].point);
        for (int i = 0; i < patch.numberOfVertices; i++) {
            Vector3D previous = new Vector3D(current.x, current.y, current.z);
            current.subtraction(patch.vertex[i].point, patch.vertex[0].point);
            normal.x += (previous.y - current.y) * (previous.z + current.z);
            normal.y += (previous.z - current.z) * (previous.x + current.x);
            normal.z += (previous.x - current.x) * (previous.y + current.y);
        }

        float localNorm = normal.norm();
        if (localNorm < Numeric.EPSILON) {
            Error.warning("patchNormal", "degenerate patch (id %d)", patch.id);
            return null;
        }
        normal.inverseScaledCopy(localNorm, normal, Numeric.EPSILON_FLOAT);
        return normal;
    }

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
            Error.error("Patch::Patch", "Null vertex");
            System.exit(1);
        }

        // It is sad but it is true
        if (inNumberOfVertices != 3 && inNumberOfVertices != 4) {
            Error.error("Patch::Patch", "Can only handle quadrilateral or triangular patches");
            System.exit(2);
        }

        Statistics.instance().reader.numberOfElements++;
        twin = null;
        id = patchId;
        patchId++;

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

        normal = new Vector3D();

        // Compute normal
        if (patchNormal(this, normal) == null) {
            Statistics.instance().reader.numberOfElements--;
            Error.error("Patch::Patch", "Error computing patch normal");
            System.exit(3);
        }

        // Also computes the jacobian
        area = computeRandomWalkRadiosityArea();

        // Patch midpoint
        midPoint = new Vector3D();
        computeMidpoint(midPoint);

        // Plane constant
        planeConstant = -normal.dotProduct(midPoint);

        // Plane tolerance
        tolerance = computeTolerance();

        // Dominant part of normal
        index = normal.dominantCoordinate();

        // Tell the vertices that there is a new Patch using them
        connectVertices();

        directPotential = 0.0f;
        color = new ColorRgb();
        color.set(0.0f, 0.0f, 0.0f);

        omit = 0;
        flags = 0; // Other flags

        radianceData = null;
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

    private static void dontIntersectBase(
        int n,
        Patch p0,
        Patch p1,
        Patch p2,
        Patch p3) {
        if (n < 0 || n > MAX_EXCLUDED_PATCHES) {
            Error.fatal(
                -1,
                "Patch::dontIntersectBase",
                "Invalid number of excluded patches %d (maximum is %d)",
                n,
                MAX_EXCLUDED_PATCHES);
            return;
        }

        Patch[] localPatches = new Patch[] {p0, p1, p2, p3};
        int i = 0;
        for (; i < n; i++) {
            excludedPatches[i] = localPatches[i];
        }
        for (; i < MAX_EXCLUDED_PATCHES; i++) {
            excludedPatches[i] = null;
        }
    }

    public static void dontIntersect0() {
        dontIntersectBase(0, null, null, null, null);
    }

    public static void dontIntersect2(Patch p0, Patch p1) {
        dontIntersectBase(2, p0, p1, null, null);
    }

    public static void dontIntersect3(Patch p0, Patch p1, Patch p2) {
        dontIntersectBase(3, p0, p1, p2, null);
    }

    public static void dontIntersect4(Patch p0, Patch p1, Patch p2, Patch p3) {
        dontIntersectBase(4, p0, p1, p2, p3);
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
                Error.fatal(-1, "textureCoordAtUv", "Invalid nr of vertices %d", numberOfVertices);
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
        RayHit hitStore) {
        RayHit hit = new RayHit();

        if (isExcluded()) {
            return null;
        }

        float distance = normal.dotProduct(ray.direction);
        if (distance > Numeric.EPSILON) {
            // Back facing patch
            if ((hitFlags & RayHitFlag.BACK) == 0) {
                return null;
            }
            int newFlags = hit.getFlags() | RayHitFlag.BACK;
            hit.setFlags(newFlags);
        }
        else if (distance < -Numeric.EPSILON) {
            // Front facing patch
            if ((hitFlags & RayHitFlag.FRONT) == 0) {
                return null;
            }
            int newFlags = hit.getFlags() | RayHitFlag.FRONT;
            hit.setFlags(newFlags);
        }
        else {
            // Ray is parallel with the plane of the patch
            return null;
        }

        distance = -(normal.dotProduct(ray.position) + planeConstant) / distance;

        if (distance > maximumDistance[0] || distance < minimumDistance) {
            // Intersection too far or too close
            return null;
        }

        // Intersection point of ray with plane of patch
        Vector3D position = new Vector3D();
        position.sumScaled(ray.position, distance, ray.direction);
        hit.setPoint(position);

        // Test whether it lies inside or outside the patch
        if (hitInPatch(hit, this)) {
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

            if ((hitFlags & RayHitFlag.UV) != 0 && (hit.getFlags() & RayHitFlag.UV) == 0) {
                position = hit.getPoint();
                double[] u = new double[] {0.0};
                double[] v = new double[] {0.0};
                hit.getPatch().uv(position, u, v);
                hit.setUv(u[0], v[0]);
                hit.setPoint(position);
                newFlags = hit.getFlags() & RayHitFlag.UV;
                hit.setFlags(newFlags);
            }

            maximumDistance[0] = distance;

            if (hitStore != null) {
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
        double a = jacobian.A;
        double b = jacobian.B;
        double c = jacobian.C;
        u[0] = ((a + 0.5 * c) + 0.5 * b * u[0]) * u[0] / area;
        v[0] = ((a + 0.5 * b) + 0.5 * c * v[0]) * v[0] / area;
    }

    /**
    Converts uniform to bi-linear coordinates.
    */
    private void uniformToBiLinear(double[] u, double[] v) {
        double a = jacobian.A;
        double b = jacobian.B;
        double c = jacobian.C;

        double A = 0.5 * b / area;
        double B = (a + 0.5 * c) / area;
        double C = -u[0];
        solveQuadraticUnitInterval(A, B, C, u);

        A = 0.5 * c / area;
        B = (a + 0.5 * b) / area;
        C = -v[0];
        solveQuadraticUnitInterval(A, B, C, v);
    }

    /**
    Given the parameter (u,v) of a point on the patch, this routine
    computes the 3D space coordinates of the same point, using barycentric
    mapping for a triangle and bi-linear mapping for a quadrilateral.
    u and v should be numbers in the range [0,1]. If u+v>1 for a triangle,
    (1-u) and (1-v) are used instead.
    */
    public Vector3D pointBarycentricMapping(double u, double v, Vector3D point) {
        if (hasZeroVertices()) {
            return null;
        }

        Vector3D v1 = vertex[0].point;
        Vector3D v2 = vertex[1].point;
        Vector3D v3 = vertex[2].point;

        if (numberOfVertices == 3) {
            if (u + v > 1.0) {
                u = 1.0 - u;
                v = 1.0 - v;
            }
            pointInTriangle(v1, v2, v3, (float)u, (float)v, point);
        }
        else if (numberOfVertices == 4) {
            Vector3D v4 = vertex[3].point;
            pointInQuadrilateral(v1, v2, v3, v4, (float)u, (float)v, point);
        }
        else {
            Error.fatal(4, "pointBarycentricMapping", "Can only handle triangular or quadrilateral patches");
        }

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
        if (jacobian != null) {
            double[] uu = new double[] {u};
            double[] vv = new double[] {v};
            uniformToBiLinear(uu, vv);
            u = uu[0];
            v = vv[0];
        }
        return pointBarycentricMapping(u, v, point);
    }

    /**
    Computes (u, v) parameters of the point on the patch (barycentric or bi-linear
    parametrisation). Returns true if the point is inside the patch and false if
    not.
    WARNING: The (u, v) coordinates are correctly computed only for positions inside
    the patch. For positions outside, they can be garbage.
    */
    public boolean uv(Vector3D point, double[] u, double[] v) {
        Vector2Dd localUv = new Vector2Dd();
        boolean inside = false;

        switch (numberOfVertices) {
            case 3:
                inside = triangleUv(point, localUv);
                break;
            case 4:
                inside = quadUv(this, point, localUv);
                break;
            default:
                Error.fatal(3, "uv", "Can only handle triangular or quadrilateral patches");
                break;
        }

        u[0] = localUv.u;
        v[0] = localUv.v;
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
    Returns true if P is at least partly in front the plane of Q. Returns false
    if P is coplanar with or behind Q. It suffices to test the vertices of P
    with respect to the plane of Q.
    */
    private boolean isAtLeastPartlyInFront(Patch other) {
        for (int i = 0; i < numberOfVertices; i++) {
            Vector3D vp = vertex[i].point;
            double ep = other.normal.dotProduct(vp) + other.planeConstant;
            double localTolerance = other.tolerance + vp.tolerance(Numeric.EPSILON_FLOAT);
            if (ep > localTolerance) {
                // P is at least partly in front of Q
                return true;
            }
        }
        return false; // P is behind or coplanar with Q
    }

    /**
    Returns true if the two patches can see each other: P and Q see each
    other if at least a part of P is in front of Q and vice versa.
    */
    public boolean facing(Patch other) {
        return isAtLeastPartlyInFront(other) && other.isAtLeastPartlyInFront(this);
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
        boolean inside = uv(point, u, v);
        if (jacobian != null) {
            biLinearToUniform(u, v);
        }
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
