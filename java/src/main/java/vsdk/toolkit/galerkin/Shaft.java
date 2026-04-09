package vsdk.toolkit.galerkin;

import java.util.ArrayList;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.RayHitFlag;
import vsdk.toolkit.scene.Polygon;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.BoundingBoxCoordinateIndex;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.GeometryClassId;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.PatchSet;
import vsdk.toolkit.skin.RayHit;

/**
References:

- [HAIN1991] Haines, E. A. and Wallace, J. R. "Shaft culling for efficient ray-traced radiosity",
  2nd Euro-graphics Workshop on Rendering, Barcelona, Spain, May 1991
*/

/**
The shaft is the region bounded by extent and referenceItem1 and referenceItem2 (if defined)
and on the negative side of the numberOfPlanesInSet.

Note: the name "Shaft" can be misleading. From [HAIN1991] it can be seen that this is more
a kind of convex envelope.
*/
public class Shaft {
    private static final int MAX_SKIP_ELEMENTS = 2;
    // Maximum 16 numberOfPlanesInSet in plane-set: maximum 8 for a box-to-box shaft (see figure [HAIN1991].2),
    // maximum 2 times the total number of vertices for a patch-to-patch shaft
    private static final int SHAFT_MAX_PLANES = 16;
    private static final int MIN_MAX_DIMENSIONS = 6;
    private static final int NONE = -1;

    private BoundingBox referenceItem1; // Bounding boxes of the reference items
    private BoundingBox referenceItem2;
    private final BoundingBox extentBoundingBox;
    private final ShaftPlane[] planeSet;
    private long numberOfPlanesInSet; // Number of planes in plane-set

    private final int[] patchIdsToOmit; // Geometries to be ignored during shaft culling, maximum 2
    private int numberOfGeometriesToOmit;
    private final int[] geometryIdsToAvoidOpening; // Geometries not to be opened during shaft culling, maximum 2
    private int numberOfGeometriesToAvoidOpen;

    private final Vector3D center1; // The line segment from center1 to center2 is guaranteed to lay within the shaft
    private final Vector3D center2;
    private boolean cut; // Full occlusion flag set when one patch cuts the shaft

    public Shaft() {
        referenceItem1 = null;
        referenceItem2 = null;
        extentBoundingBox = new BoundingBox();
        planeSet = new ShaftPlane[SHAFT_MAX_PLANES];
        for (int i = 0; i < planeSet.length; i++) {
            planeSet[i] = new ShaftPlane();
        }
        numberOfPlanesInSet = 0;
        patchIdsToOmit = new int[MAX_SKIP_ELEMENTS];
        geometryIdsToAvoidOpening = new int[MAX_SKIP_ELEMENTS];
        numberOfGeometriesToOmit = 0;
        numberOfGeometriesToAvoidOpen = 0;
        center1 = new Vector3D();
        center2 = new Vector3D();
        cut = false;
    }

    public static void freeCandidateList(ArrayList<Geometry> candidateList) {
        // Only destroy geometries that were generated for shaft culling
        for (int i = 0; candidateList != null && i < candidateList.size(); i++) {
            Geometry geometry = candidateList.get(i);
            if (geometry != null && geometry.shaftCullGeometry) {
                Geometry.destroy(geometry);
            }
        }
        if (candidateList != null) {
            candidateList.clear();
        }
    }

    public boolean isCut() {
        return cut;
    }

    /**
    Marks a geometry as to be omitted during shaft culling: it will not be added to the
    candidate list, even if the geometry overlaps or is inside the shaft.
    */
    public void setShaftOmit(Patch patch) {
        if (patch == null || numberOfGeometriesToOmit >= MAX_SKIP_ELEMENTS) {
            return;
        }
        patchIdsToOmit[numberOfGeometriesToOmit++] = patch.id;
    }

    /**
    Marks a geometry as one not to be opened during shaft culling.
    */
    public void setShaftDontOpen(Geometry geometry) {
        if (geometry == null || numberOfGeometriesToAvoidOpen >= MAX_SKIP_ELEMENTS) {
            return;
        }
        geometryIdsToAvoidOpening[numberOfGeometriesToAvoidOpen++] = geometry.id;
    }

    /**
    Constructs a shaft for two given bounding boxes.
    */
    public void constructFromBoundingBoxes(BoundingBox boundingBox1, BoundingBox boundingBox2) {
        numberOfGeometriesToOmit = 0;
        numberOfGeometriesToAvoidOpen = 0;
        cut = false;

        // 1. Obtain the bounding boxes for the reference items [HAIN1991]
        referenceItem1 = boundingBox1;
        referenceItem2 = boundingBox2;
        if (referenceItem1 == null || referenceItem2 == null) {
            numberOfPlanesInSet = 0;
            return;
        }

        // Midpoints of the reference boxes define a line that is guaranteed to lay within the shaft
        center1.copy(boundingBox1.center());
        center2.copy(boundingBox2.center());

        // 2. Compute the extent bounding box containing both reference items [HAIN1991]
        boolean[] hasMinMax1 = new boolean[MIN_MAX_DIMENSIONS]; // Representation of culled edges for extent box
        boolean[] hasMinMax2 = new boolean[MIN_MAX_DIMENSIONS];
        for (int i = 0; i < MIN_MAX_DIMENSIONS; i++) {
            hasMinMax1[i] = false;
            hasMinMax2[i] = false;
        }

        extentBoundingBox.setAsUnion(referenceItem1, referenceItem2);
        referenceItem1.computeContributionFlags(referenceItem2, hasMinMax1, hasMinMax2);

        // 3. Create the plane set between the two reference items' boxes [HAIN1991]
        int localPlaneIndex = 0;
        for (int i = 0; i < MIN_MAX_DIMENSIONS; i++) {
            if (!hasMinMax1[i]) {
                continue;
            }

            for (int j = 0; j < MIN_MAX_DIMENSIONS; j++) {
                // 3.1. Compute plane normal for marked borders
                int a = i % 3; // Directions
                int b = j % 3;

                if (!hasMinMax2[j] || a == b) {
                    // Same direction
                    continue;
                }

                float u1 = referenceItem1.valueAt(i);
                float v1 = referenceItem1.valueAt(j);
                float u2 = referenceItem2.valueAt(i);
                float v2 = referenceItem2.valueAt(j);

                float du;
                float dv;

                if ((i <= BoundingBoxCoordinateIndex.MIN_Z && j <= BoundingBoxCoordinateIndex.MIN_Z)
                    || (i >= BoundingBoxCoordinateIndex.MAX_X && j >= BoundingBoxCoordinateIndex.MAX_X)) {
                    du = v2 - v1;
                    dv = u1 - u2;
                }
                else {
                    // Normal must point outwards shaft
                    du = v1 - v2;
                    dv = u2 - u1;
                }

                // 3.2. Build the newly identified plane
                ShaftPlane localPlane = planeSet[localPlaneIndex];
                localPlane.n[a] = du;
                localPlane.n[b] = dv;
                localPlane.n[3 - a - b] = 0.0f;
                float dExpr = -(du * u1 + dv * v1);
                float dResolved = dExpr;
                // Preserve tiny cancellation residuals observed in C++ for near-zero d.
                if (Math.abs(dResolved) <= 1.0e-10f) {
                    float dFma = -Math.fma(dv, v1, du * u1);
                    if (dFma != 0.0f && Math.abs(dFma) <= 1.0e-5f) {
                        dResolved = dFma;
                    }
                }
                localPlane.d = dResolved;

                localPlane.coordinateOffset[0] =
                    localPlane.n[0] > 0.0f ? BoundingBoxCoordinateIndex.MIN_X : BoundingBoxCoordinateIndex.MAX_X;
                localPlane.coordinateOffset[1] =
                    localPlane.n[1] > 0.0f ? BoundingBoxCoordinateIndex.MIN_Y : BoundingBoxCoordinateIndex.MAX_Y;
                localPlane.coordinateOffset[2] =
                    localPlane.n[2] > 0.0f ? BoundingBoxCoordinateIndex.MIN_Z : BoundingBoxCoordinateIndex.MAX_Z;

                localPlaneIndex++;
            }
        }
        numberOfPlanesInSet = localPlaneIndex;
    }

    /**
    Tests a polygon with respect to the plane defined by the given normal and plane
    constant. Returns INSIDE if the polygon is totally on the negative side of
    the plane, OUTSIDE if the polygon is all on the positive side, OVERLAP
    if the polygon is cut by the plane and COPLANAR if the polygon lies on the
    plane within tolerance distance d*Numeric.EPSILON.
    */
    private static ShaftPlanePosition testPolygonWithRespectToPlane(Polygon polygon, Vector3D normal, double d) {
        boolean out = false; // out = there are positions on the positive side of the plane
        boolean in = false; // in  = there are positions on the negative side of the plane

        for (int i = 0; i < polygon.numberOfVertices; i++) {
            double e = normal.dotProduct(polygon.vertex[i]) + d;
            double tolerance = Math.abs(d) * Numeric.EPSILON + polygon.vertex[i].tolerance(Numeric.EPSILON_FLOAT);
            out |= (e > tolerance);
            in |= (e < -tolerance);
            if (out && in) {
                return ShaftPlanePosition.OVERLAP;
            }
        }

        if (out) {
            return ShaftPlanePosition.OUTSIDE;
        }
        return in ? ShaftPlanePosition.INSIDE : ShaftPlanePosition.COPLANAR;
    }

    /**
    Verifies whether the polygon is on the given side of the plane. Returns true if
    so, and false if not.
    */
    private static boolean verifyPolygonWithRespectToPlane(
        Polygon polygon,
        Vector3D normal,
        double d,
        ShaftPlanePosition side)
    {
        boolean out = false;
        boolean in = false;

        for (int i = 0; i < polygon.numberOfVertices; i++) {
            double e = normal.dotProduct(polygon.vertex[i]) + d;
            double tolerance = Math.abs(d) * Numeric.EPSILON + polygon.vertex[i].tolerance(Numeric.EPSILON_FLOAT);
            out |= e > tolerance;
            if (out && side == ShaftPlanePosition.INSIDE) {
                return false;
            }
            in |= e < -tolerance;
            if (in && side == ShaftPlanePosition.OUTSIDE) {
                return false;
            }
        }

        if (in) {
            if (side == ShaftPlanePosition.INSIDE) {
                return true;
            }
        }
        else if (out) {
            if (side == ShaftPlanePosition.OUTSIDE) {
                return true;
            }
        }
        return false;
    }

    /**
    Tests the position of a point with respect to a plane. Returns OUTSIDE if the point is
    on the positive side of the plane, INSIDE if on the negative side, and COPLANAR
    if the point is on the plane within tolerance distance d*Numeric.EPSILON.
    */
    private static ShaftPlanePosition testPointWithRespectToPlane(Vector3D p, Vector3D normal, double d) {
        double tolerance = Math.abs(d * Numeric.EPSILON) + p.tolerance(Numeric.EPSILON_FLOAT);
        double e = normal.dotProduct(p) + d;
        if (e < -tolerance) {
            return ShaftPlanePosition.INSIDE;
        }
        if (e > +tolerance) {
            return ShaftPlanePosition.OUTSIDE;
        }
        return ShaftPlanePosition.COPLANAR;
    }

    /**
    Compare two shaft planes. Returns 0 if they are the same and -1 or +1
    if not. It is assumed that plane normals are normalized.
    */
    private static int compareShaftPlanes(ShaftPlane plane1, ShaftPlane plane2) {
        // Compare components of plane normal (normalized vector, so components
        // are in the range [-1, 1]).
        if (plane1.n[0] < plane2.n[0] - Numeric.EPSILON) {
            return -1;
        }
        else if (plane1.n[0] > plane2.n[0] + Numeric.EPSILON) {
            return +1;
        }

        if (plane1.n[1] < plane2.n[1] - Numeric.EPSILON) {
            return -1;
        }
        else if (plane1.n[1] > plane2.n[1] + Numeric.EPSILON) {
            return +1;
        }

        if (plane1.n[2] < plane2.n[2] - Numeric.EPSILON) {
            return -1;
        }
        else if (plane1.n[2] > plane2.n[2] + Numeric.EPSILON) {
            return +1;
        }

        // Compare plane constants
        double tolerance = Math.abs(Math.max(plane1.d, plane2.d) * Numeric.EPSILON);
        if (plane1.d < plane2.d - tolerance) {
            return -1;
        }
        else if (plane1.d > plane2.d + tolerance) {
            return +1;
        }
        return 0;
    }

    /**
    Plane is a reference to one of the shaft planes. Returns true if the plane
    differs from all previous defined planes.
    */
    private boolean uniqueShaftPlane(ShaftPlane parameterPlane) {
        for (int i = 0; planeSet[i] != parameterPlane; i++) {
            if (compareShaftPlanes(planeSet[i], parameterPlane) == 0) {
                return false;
            }
        }
        return true;
    }

    /**
    Fills in normal and plane constant, as well as coordinate offset parameters.
    */
    private static void fillInPlane(ShaftPlane plane, float nx, float ny, float nz, float d) {
        plane.n[0] = nx;
        plane.n[1] = ny;
        plane.n[2] = nz;
        plane.d = d;

        plane.coordinateOffset[0] = plane.n[0] > 0.0f ? BoundingBoxCoordinateIndex.MIN_X : BoundingBoxCoordinateIndex.MAX_X;
        plane.coordinateOffset[1] = plane.n[1] > 0.0f ? BoundingBoxCoordinateIndex.MIN_Y : BoundingBoxCoordinateIndex.MAX_Y;
        plane.coordinateOffset[2] = plane.n[2] > 0.0f ? BoundingBoxCoordinateIndex.MIN_Z : BoundingBoxCoordinateIndex.MAX_Z;
    }

    /**
    Construct the planes determining the shaft that use edges of polygon1 and vertices of polygon2.
    */
    private void constructPolygonToPolygonPlanes(Polygon polygon1, Polygon polygon2) {
        Vector3D normal = new Vector3D();
        int localPlaneIndex = (int)numberOfPlanesInSet;
        int maxPlanesPerEdge;

        // Test polygon2 wrt plane of polygon1
        normal.copy(polygon1.normal); // Convert to double precision equivalent
        switch (testPolygonWithRespectToPlane(polygon2, normal, polygon1.planeConstant)) {
            case INSIDE:
                // polygon2 is on negative side of plane of polygon1.
                fillInPlane(
                    planeSet[localPlaneIndex],
                    polygon1.normal.x, polygon1.normal.y, polygon1.normal.z, polygon1.planeConstant);
                if (uniqueShaftPlane(planeSet[localPlaneIndex])) {
                    localPlaneIndex++;
                }
                maxPlanesPerEdge = 1;
                break;
            case OUTSIDE:
                // polygon2 is on positive side of plane of polygon1; invert normal and constant.
                fillInPlane(
                    planeSet[localPlaneIndex],
                    -polygon1.normal.x, -polygon1.normal.y, -polygon1.normal.z, -polygon1.planeConstant);
                if (uniqueShaftPlane(planeSet[localPlaneIndex])) {
                    localPlaneIndex++;
                }
                maxPlanesPerEdge = 1;
                break;
            case OVERLAP:
                // Plane of polygon1 cuts polygon2: up to two shaft planes per edge.
                maxPlanesPerEdge = 2;
                break;
            default:
                // COPLANAR degenerate case
                return;
        }

        for (int i = 0; i < polygon1.numberOfVertices; i++) {
            // For each edge of polygon1
            Vector3D current = polygon1.vertex[i];
            Vector3D next = polygon1.vertex[(i + 1) % polygon1.numberOfVertices];
            int planesFoundForEdge = 0;

            for (int j = 0; j < polygon2.numberOfVertices && planesFoundForEdge < maxPlanesPerEdge; j++) {
                // For each vertex of polygon2
                Vector3D other = polygon2.vertex[j];

                // Compute normal and plane constant of plane formed by current, next and other
                normal.tripleCrossProduct(current, next, other);
                float localNorm = normal.norm();
                if (localNorm < Numeric.EPSILON) {
                    continue;
                }
                // Co-linear vertices, try next vertex on polygon2
                normal.inverseScaledCopy(localNorm, normal, Numeric.EPSILON_FLOAT);
                float dNaive = -normal.dotProduct(current);
                float dExtended =
                    (float)(-((double)normal.x * (double)current.x
                        + (double)normal.y * (double)current.y
                        + (double)normal.z * (double)current.z));
                float dFma = -Math.fma(normal.z, current.z, normal.x * current.x + normal.y * current.y);
                float d = dNaive;
                if (Math.abs(d) <= 1.0e-10f) {
                    if (dFma != 0.0f && Math.abs(dFma) <= 1.0e-5f) {
                        d = dFma;
                    }
                    else if (dExtended != 0.0f && Math.abs(dExtended) <= 1.0e-5f) {
                        d = Math.abs(dExtended);
                    }
                }

                // Test position of polygon1 with respect to constructed plane.
                ShaftPlanePosition side =
                    testPointWithRespectToPlane(polygon1.vertex[(i + 2) % polygon1.numberOfVertices], normal, d);
                for (int k = (i + 3) % polygon1.numberOfVertices; k != i; k = (k + 1) % polygon1.numberOfVertices) {
                    ShaftPlanePosition nSide = testPointWithRespectToPlane(polygon1.vertex[k], normal, d);
                    if (side == ShaftPlanePosition.COPLANAR) {
                        side = nSide;
                    }
                    else if (nSide != ShaftPlanePosition.COPLANAR && side != nSide) {
                        // side==INSIDE and nSide==OUTSIDE or vice versa
                        side = ShaftPlanePosition.OVERLAP;
                    }
                }
                if (side != ShaftPlanePosition.INSIDE && side != ShaftPlanePosition.OUTSIDE) {
                    continue; // Not a valid candidate shaft plane
                }

                // Verify whether polygon2 is on the same side of constructed plane.
                if (verifyPolygonWithRespectToPlane(polygon2, normal, d, side)) {
                    if (side == ShaftPlanePosition.INSIDE) {
                        // polygon1 and polygon2 are on negative side as it should be
                        fillInPlane(planeSet[localPlaneIndex], normal.x, normal.y, normal.z, d);
                    }
                    else {
                        fillInPlane(planeSet[localPlaneIndex], -normal.x, -normal.y, -normal.z, -d);
                    }
                    if (uniqueShaftPlane(planeSet[localPlaneIndex])) {
                        localPlaneIndex++;
                    }
                    planesFoundForEdge++;
                }
            }
        }

        numberOfPlanesInSet = localPlaneIndex;
    }

    /**
    Constructs a shaft enclosing the two given polygons.
    */
    public void constructFromPolygonToPolygon(Polygon polygon1, Polygon polygon2) {
        // No reference bounding boxes to test with
        referenceItem1 = null;
        referenceItem2 = null;

        // Shaft extent: bounding box containing the bounding boxes of the polygons
        extentBoundingBox.copyFrom(polygon1.bounds);
        extentBoundingBox.enlarge(polygon2.bounds);

        // Nothing (yet) to omit
        patchIdsToOmit[0] = NONE;
        patchIdsToOmit[1] = NONE;
        geometryIdsToAvoidOpening[0] = NONE;
        geometryIdsToAvoidOpening[1] = NONE;
        numberOfGeometriesToOmit = 0;
        numberOfGeometriesToAvoidOpen = 0;
        cut = false;

        // Center positions of polygons define a line guaranteed to lie inside shaft
        center1.copy(polygon1.vertex[0]);
        for (int i = 1; i < polygon1.numberOfVertices; i++) {
            center1.addition(center1, polygon1.vertex[i]);
        }
        center1.inverseScaledCopy((float)polygon1.numberOfVertices, center1, Numeric.EPSILON_FLOAT);

        center2.copy(polygon2.vertex[0]);
        for (int i = 1; i < polygon2.numberOfVertices; i++) {
            center2.addition(center2, polygon2.vertex[i]);
        }
        center2.inverseScaledCopy((float)polygon2.numberOfVertices, center2, Numeric.EPSILON_FLOAT);

        // Determine shaft planes
        numberOfPlanesInSet = 0;
        constructPolygonToPolygonPlanes(polygon1, polygon2);
        constructPolygonToPolygonPlanes(polygon2, polygon1);
    }

    /**
    Tests a bounding volume against the shaft: returns INSIDE if inside shaft,
    OVERLAP if it overlaps, OUTSIDE if it is outside shaft.
    */
    private static float evaluatePlane(ShaftPlane plane, float x, float y, float z) {
        return Math.fma(plane.n[1], y, Math.fma(plane.n[0], x, Math.fma(plane.n[2], z, plane.d)));
    }

    private ShaftPlanePosition boundingBoxTest(BoundingBox parameterBoundingBox) {
        // Test against extent box
        if (parameterBoundingBox.disjointToOtherBoundingBox(extentBoundingBox)) {
            return ShaftPlanePosition.OUTSIDE;
        }

        // Test against plane set: if nearest corner is on/outside any shaft plane, object is outside
        for (int i = 0; i < numberOfPlanesInSet; i++) {
            ShaftPlane localPlane = planeSet[i];
            float e = evaluatePlane(
                localPlane,
                parameterBoundingBox.valueAt(localPlane.coordinateOffset[0]),
                parameterBoundingBox.valueAt(localPlane.coordinateOffset[1]),
                parameterBoundingBox.valueAt(localPlane.coordinateOffset[2]));
            if (e > -Math.abs(localPlane.d * Numeric.EPSILON)) {
                return ShaftPlanePosition.OUTSIDE;
            }
        }

        // Test against reference bounding boxes
        if ((referenceItem1 != null && !parameterBoundingBox.disjointToOtherBoundingBox(referenceItem1))
            || (referenceItem2 != null && !parameterBoundingBox.disjointToOtherBoundingBox(referenceItem2))) {
            return ShaftPlanePosition.OVERLAP;
        }

        // If farthest corner is outside any shaft plane, it overlaps; otherwise inside
        for (int i = 0; i < numberOfPlanesInSet; i++) {
            ShaftPlane localPlane = planeSet[i];
            float e = evaluatePlane(
                localPlane,
                parameterBoundingBox.valueAt((localPlane.coordinateOffset[0] + 3) % 6),
                parameterBoundingBox.valueAt((localPlane.coordinateOffset[1] + 3) % 6),
                parameterBoundingBox.valueAt((localPlane.coordinateOffset[2] + 3) % 6));
            if (e > Math.abs(localPlane.d * Numeric.EPSILON)) {
                return ShaftPlanePosition.OVERLAP;
            }
        }

        return ShaftPlanePosition.INSIDE;
    }

    /**
    Tests a patch against shaft: INSIDE, OVERLAP or OUTSIDE.
    Sets cut=true if full occlusion is detected.
    */
    private ShaftPlanePosition shaftPatchTest(Patch patch) {
        int[] inAll = new int[Patch.MAXIMUM_VERTICES_PER_PATCH];
        double[] tMin = new double[Patch.MAXIMUM_VERTICES_PER_PATCH];
        double[] tMax = new double[Patch.MAXIMUM_VERTICES_PER_PATCH];
        double[] pTol = new double[Patch.MAXIMUM_VERTICES_PER_PATCH];
        Ray ray = new Ray();
        RayHit hitStore = new RayHit();

        // Start by assuming all vertices are inside all shaft planes
        boolean someOut = false;
        for (int j = 0; j < patch.numberOfVertices; j++) {
            inAll[j] = 1;
            tMin[j] = 0.0;
            tMax[j] = 1.0;
            pTol[j] = patch.vertex[j].point.tolerance(Numeric.EPSILON_FLOAT);
        }

        for (int i = 0; i < numberOfPlanesInSet; i++) {
            // Test patch against i-th shaft plane
            ShaftPlane localPlane = planeSet[i];
            double[] e = new double[Patch.MAXIMUM_VERTICES_PER_PATCH];
            ShaftPlanePosition[] side = new ShaftPlanePosition[Patch.MAXIMUM_VERTICES_PER_PATCH];
            boolean in = false;
            boolean out = false;

            for (int j = 0; j < patch.numberOfVertices; j++) {
                Vector3D v = patch.vertex[j].point;
                e[j] = evaluatePlane(localPlane, v.x, v.y, v.z);
                double tolerance = Math.abs(localPlane.d) * Numeric.EPSILON + pTol[j];
                side[j] = ShaftPlanePosition.COPLANAR;
                if (e[j] > tolerance) {
                    side[j] = ShaftPlanePosition.OUTSIDE;
                    out = true;
                }
                else if (e[j] < -tolerance) {
                    side[j] = ShaftPlanePosition.INSIDE;
                    in = true;
                }
                if (side[j] != ShaftPlanePosition.INSIDE) {
                    inAll[j] = 0;
                }
            }

            if (!in) {
                // Patch contains no vertices on inside side of plane
                return ShaftPlanePosition.OUTSIDE;
            }

            if (out) {
                // At least one vertex inside and one outside this plane
                someOut = true;

                for (int j = 0; j < patch.numberOfVertices; j++) {
                    // Reduce segment of edge that can lie within shaft
                    int k = (j + 1) % patch.numberOfVertices;
                    if (side[j] != side[k]) {
                        if (side[k] == ShaftPlanePosition.OUTSIDE) {
                            // Decrease tMax[j]
                            if (side[j] == ShaftPlanePosition.INSIDE) {
                                if (tMax[j] > tMin[j]) {
                                    double t = e[j] / (e[j] - e[k]);
                                    if (t < tMax[j]) {
                                        tMax[j] = t;
                                    }
                                }
                            }
                            else {
                                // side[j] == COPLANAR, whole edge lies outside
                                tMax[j] = -Numeric.EPSILON;
                            }
                        }
                        else if (side[j] == ShaftPlanePosition.OUTSIDE) {
                            // Increase tMin[j]
                            if (side[k] == ShaftPlanePosition.INSIDE) {
                                if (tMin[j] < tMax[j]) {
                                    double t = e[j] / (e[j] - e[k]);
                                    if (t > tMin[j]) {
                                        tMin[j] = t;
                                    }
                                }
                            }
                            else {
                                // side[k] == COPLANAR, whole edge lies outside
                                tMin[j] = 1.0 + Numeric.EPSILON;
                            }
                        }
                    }
                    else if (side[j] == ShaftPlanePosition.OUTSIDE) {
                        // Whole edge lies outside
                        tMax[j] = -Numeric.EPSILON;
                    }
                }
            }
        }

        // Remaining tests only work if shaft planes alone determine shaft
        if (referenceItem1 != null || referenceItem2 != null) {
            return ShaftPlanePosition.OVERLAP;
        }

        if (!someOut) {
            // No patch vertices are outside shaft
            return ShaftPlanePosition.INSIDE;
        }

        for (int j = 0; j < patch.numberOfVertices; j++) {
            if (inAll[j] != 0) {
                // At least one patch vertex is really inside shaft
                return ShaftPlanePosition.OVERLAP;
            }
        }

        // All vertices are outside or on shaft. Check whether edges intersect shaft
        for (int j = 0; j < patch.numberOfVertices; j++) {
            if (tMin[j] + Numeric.EPSILON < tMax[j] - Numeric.EPSILON) {
                return ShaftPlanePosition.OVERLAP;
            }
        }

        // All vertices and edges are outside shaft. Either patch is fully outside,
        // or it cuts the shaft.
        ray.position.copy(center1);
        ray.direction.subtraction(center2, center1);
        float[] dist = new float[] {1.0f - Numeric.EPSILON_FLOAT};
        if (patch.intersect(
            ray,
            Numeric.EPSILON_FLOAT,
            dist,
            RayHitFlag.FRONT | RayHitFlag.BACK,
            hitStore) != null) {
            cut = true;
            return ShaftPlanePosition.OVERLAP;
        }

        return ShaftPlanePosition.OUTSIDE;
    }

    /**
    Returns true if the patch id is to be omitted during shaft culling.
    */
    private boolean patchIsOnOmitSet(int id) {
        for (int i = 0; i < numberOfGeometriesToOmit && i < MAX_SKIP_ELEMENTS; i++) {
            if (patchIdsToOmit[i] == id) {
                return true;
            }
        }
        return false;
    }

    /**
    Returns true if geometry is not to be opened during shaft culling.
    */
    private boolean closedGeometry(Geometry geometry) {
        for (int i = 0; i < numberOfGeometriesToAvoidOpen && i < MAX_SKIP_ELEMENTS; i++) {
            if (geometryIdsToAvoidOpening[i] == geometry.id) {
                return true;
            }
        }
        return false;
    }

    /**
    Given a patch list, checks every patch and returns a culled list
    containing inside or overlapping patches.
    */
    private ArrayList<Patch> cullPatches(ArrayList<Patch> patchList) {
        ArrayList<Patch> culledPatchList = new ArrayList<>();

        for (int i = 0; patchList != null && i < patchList.size() && !cut; i++) {
            Patch patch = patchList.get(i);
            if (patch.omit != 0 || patchIsOnOmitSet(patch.id)) {
                continue;
            }

            if (patch.boundingBox == null) {
                patch.computeBoundingBox();
            }

            ShaftPlanePosition boundingBoxSide = boundingBoxTest(patch.boundingBox);
            // If patch bounding box overlaps, do definitive patch test.
            if (boundingBoxSide != ShaftPlanePosition.OUTSIDE
                && (boundingBoxSide == ShaftPlanePosition.INSIDE
                    || shaftPatchTest(patch) != ShaftPlanePosition.OUTSIDE)) {
                culledPatchList.add(patch);
            }
        }
        return culledPatchList;
    }

    /**
    Adds geometry to candidate list, duplicating if it was created during previous shaft culling.
    */
    private static void keep(Geometry geometry, ArrayList<Geometry> candidateList) {
        if (geometry == null || candidateList == null || geometry.omit) {
            return;
        }

        if (geometry.shaftCullGeometry && geometry.className == GeometryClassId.PATCH_SET) {
            // TODO: Should be PatchSet, or implement clone in all Geometry subclasses.
            Geometry newGeometry = geometry.clone();
            newGeometry.shaftCullGeometry = true;
            candidateList.add(newGeometry);
        }
        else {
            candidateList.add(geometry);
        }
    }

    /**
    Breaks geometry into components and does shaft culling on components.
    */
    private void shaftCullOpen(Geometry geometry, ArrayList<Geometry> candidateList, ShaftCullStrategy strategy) {
        if (geometry == null || candidateList == null || geometry.omit) {
            return;
        }

        if (geometry.isCompound()) {
            Compound compound = (Compound)geometry;
            doCulling(compound.children, candidateList, strategy);
        }
        else {
            ArrayList<Patch> geometryPatchesList = Geometry.patchListReference(geometry);
            ArrayList<Patch> culledPatches = cullPatches(geometryPatchesList);

            if (culledPatches.size() > 0) {
                PatchSet newGeometry = new PatchSet(culledPatches);
                newGeometry.shaftCullGeometry = true;
                newGeometry.isDuplicate = false;
                candidateList.add(newGeometry);
            }
            culledPatches.clear();
        }
    }

    /**
    Tests geometry with respect to shaft. If inside or overlapping, it is either
    copied as-is or opened depending on shaft culling strategy.
    */
    public void cullGeometry(Geometry geometry, ArrayList<Geometry> candidateList, ShaftCullStrategy strategy) {
        if (geometry == null || candidateList == null) {
            return;
        }

        if (geometry.className == GeometryClassId.PATCH_SET) {
            // Keep parity with C++ behavior where this path effectively probes omit set with id 1.
            int patchId = 1;
            if (geometry.omit || patchIsOnOmitSet(patchId)) {
                return;
            }
        }

        ShaftPlanePosition side = geometry.bounded ? boundingBoxTest(geometry.boundingBox) : ShaftPlanePosition.OVERLAP;
        switch (side) {
            case INSIDE:
                if (strategy == ShaftCullStrategy.ALWAYS_OPEN && !closedGeometry(geometry)) {
                    shaftCullOpen(geometry, candidateList, strategy);
                }
                else {
                    keep(geometry, candidateList);
                }
                break;
            case OVERLAP:
                if (closedGeometry(geometry)) {
                    keep(geometry, candidateList);
                }
                else {
                    shaftCullOpen(geometry, candidateList, strategy);
                }
                break;
            default:
                break;
        }
    }

    /**
    Adds all objects from world that overlap or lie inside the shaft to candidate list.
    */
    public void doCulling(ArrayList<Geometry> world, ArrayList<Geometry> candidateList, ShaftCullStrategy strategy) {
        if (candidateList == null) {
            return;
        }
        for (int i = 0; world != null && i < world.size() && !cut; i++) {
            cullGeometry(world.get(i), candidateList, strategy);
        }
    }
}
