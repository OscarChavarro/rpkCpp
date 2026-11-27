//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

import java.util.ArrayList;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.Geometry;
import vsdk.toolkit.environment.geometry.surface.InfinitePlane;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidHalfEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidLoop;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidVertex;

/**
Geometric validation helpers for polyhedral B-Reps, centered on the planar
face model of [MANT1988].10.2.1 and the geometric primitives developed in
chapter [MANT1988].13.
*/
public class PolyhedralBoundedSolidGeometricValidator
{
    private PolyhedralBoundedSolidGeometricValidator()
    {
    }

    /**
    Checks whether a set of face vertices can define the planar polygon
    required by [MANT1988].10.2.1 and by the face-equation procedure of
    [MANT1988].13.1.
    */
    public static boolean validateFacePointsAreCoplanar(ArrayList<Vector3Dd> points)
    {
        return validateFacePointsAreCoplanar(points,
            PolyhedralBoundedSolidNumericPolicy.forPoints(points));
    }

    /**
    Implements the coplanarity precondition needed before applying the face
    equation ideas of [MANT1988].13.1 to a polyhedral face from
    [MANT1988].10.2.1.
    */
    public static boolean validateFacePointsAreCoplanar(ArrayList<Vector3Dd> points,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext)
    {
        if ( points == null || points.size() < 3 ) {
            return false;
        }
        if ( numericContext == null ) {
            numericContext = PolyhedralBoundedSolidNumericPolicy.defaultContext();
        }

        Vector3Dd p0;
        Vector3Dd p1;
        Vector3Dd p2;
        p0 = points.get(0);
        boolean foundSeparatedPair = false;

        int i;
        for ( i = 1; i < points.size(); i++ ) {
            p1 = points.get(i);
            if ( PolyhedralBoundedSolidNumericPolicy.pointsSeparated(
                p0, p1, numericContext) ) {
                foundSeparatedPair = true;
                break;
            }
        }
        if ( !foundSeparatedPair ) {
            return false;
        }

        Vector3Dd a;
        Vector3Dd b;
        Vector3Dd n;
        double aDotB;
        InfinitePlane facePlane = null;
        int j;
        int k;

        // The plane is the one of the last pair (i, j), in ascending
        // order, whose first admissible third point k is not collinear
        // with it. The pairs are visited from the last one backwards and
        // the search stops at the first that gives a plane: the same
        // plane as visiting every pair and keeping the last one, which
        // took a time cubic in the number of points (performance.md).
        for ( i = points.size() - 1; i >= 0 && facePlane == null; i-- ) {
            for ( j = points.size() - 1; j >= 0 && facePlane == null; j-- ) {
                for ( k = 0; k < points.size(); k++ ) {
                    if ( i == j || i == k || j == k ) {
                        continue;
                    }
                    p0 = points.get(i);
                    p1 = points.get(j);
                    p2 = points.get(k);
                    if ( PolyhedralBoundedSolidNumericPolicy.pointsSeparated(
                             p0, p2, numericContext) &&
                         PolyhedralBoundedSolidNumericPolicy.pointsSeparated(
                             p1, p2, numericContext) ) {
                        a = p2.subtract(p0);
                        b = p1.subtract(p0);
                        a = a.normalized();
                        b = b.normalized();
                        aDotB = Math.abs(a.dotProduct(b));
                        if ( aDotB < 1.0 - numericContext.unitVectorTolerance() ) {
                            n = a.crossProduct(b);
                            n = n.normalized();
                            facePlane = new InfinitePlane(n, p0);
                        }
                        break;
                    }
                }
            }
        }

        if ( facePlane == null ) {
            return false;
        }

        for ( i = 1; i < points.size(); i++ ) {
            p0 = points.get(i);
            if ( facePlane.doContainmentTest(p0, numericContext.epsilon()) !=
                 Geometry.LIMIT ) {
                return false;
            }
        }

        return true;
    }

    public static ArrayList<Vector3Dd> extractPointsFromFace(_PolyhedralBoundedSolidFace face)
    {
        boolean test = true;
        ArrayList<Vector3Dd> points;
        points = new ArrayList<Vector3Dd>();
        int j;

        for ( j = 0; j < face.boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop loop;
            _PolyhedralBoundedSolidHalfEdge he;
            _PolyhedralBoundedSolidHalfEdge heStart;

            loop = face.boundariesList.get(j);
            he = loop.boundaryStartHalfEdge;
            if ( he == null ) {
                test = false;
                break;
            }
            heStart = he;
            do {
                he = he.next();
                if ( he == null ) {
                    test = false;
                    break;
                }
                points.add(he.startingVertex.position);
            } while( he != heStart );
        }

        if ( !test ) {
            return null;
        }
        return points;
    }

    /**
    Validates that a face instance matches the planar-face assumption from
    [MANT1988].10.2.1.
    */
    public static boolean validateFaceIsPlanar(_PolyhedralBoundedSolidFace face)
    {
        return validateFaceIsPlanar(face,
            PolyhedralBoundedSolidNumericPolicy.forFace(face));
    }

    /**
    Validates the face-planarity invariant required before evaluating face
    equations as in [MANT1988].13.1.
    */
    public static boolean validateFaceIsPlanar(_PolyhedralBoundedSolidFace face,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext)
    {
        ArrayList<Vector3Dd> points = extractPointsFromFace(face);
        return (points != null) &&
            validateFacePointsAreCoplanar(points, numericContext);
    }

    /**
    Rebuilds the plane equations expected for face nodes in [MANT1988].10.2.1
    by first checking planarity and then evaluating planes in the spirit of
    [MANT1988].13.1.
    */
    public static boolean validateAllFacesPlanarityAndPlanes(
        PolyhedralBoundedSolid solid, StringBuilder msg)
    {
        return validateAllFacesPlanarityAndPlanes(solid,
            PolyhedralBoundedSolidNumericPolicy.forSolid(solid), msg);
    }

    /**
    Applies the face-planarity and face-equation checks corresponding to
    [MANT1988].10.2.1 and [MANT1988].13.1 to every face of the solid.
    */
    public static boolean validateAllFacesPlanarityAndPlanes(
        PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        int i;
        boolean test = true;
        for ( i = 0; i < solid.getPolygonsList().size(); i++ ) {
            _PolyhedralBoundedSolidFace face = solid.getPolygonsList().get(i);
            if ( validateFaceIsPlanar(face, numericContext) ) {
                if ( face.getContainingPlane() == null ) {
                    msg.append("  - Face [").append(face.id)
                       .append("] was not able to compute containing plane\n");
                    test = false;
                }
            }
            else {
                msg.append("  - Face [").append(face.id).append("] is not coplanar\n");
                test = false;
            }
        }
        return test;
    }

    /**
    Convenience overload that derives the tolerance context from the solid.
    See {@link #validateConsistentFaceOrientations(PolyhedralBoundedSolid,
    PolyhedralBoundedSolidNumericPolicy.ToleranceContext, StringBuilder)}.
    */
    public static boolean validateConsistentFaceOrientations(
        PolyhedralBoundedSolid solid, StringBuilder msg)
    {
        return validateConsistentFaceOrientations(solid,
            PolyhedralBoundedSolidNumericPolicy.forSolid(solid), msg);
    }

    /**
    Heuristic check for the consistent face-orientation invariant required by
    the 2-manifold boundary model of [MANT1988].10.2.1.  Instead of using a
    global centroid (which produces false positives on hollow shells like
    the Kurlander bowl), this routine compares each face plane normal
    against the normals of its topological neighbours through shared edges.
    An inverted face presents a normal that is strongly anti-parallel to
    every one of its neighbours (cos > 120 degrees, i.e. dot product
    smaller than `-INVERTED_FACE_THRESHOLD`).  Smooth curved surfaces
    (cos < 30 degrees), sharp creases (cos around 0) and even concave
    edges (cos > -0.5) all stay above the threshold; only a flipped
    triangle whose normal opposes every neighbour gets flagged.

    Limitation: still a heuristic.  A face that happens to be isolated
    (no manifold neighbours) is skipped.  False negatives are possible
    when an invariant inversion affects an entire connected patch
    consistently; for the Kurlander bowl case the inverted faces are
    always isolated triangles produced by fan-triangulation, so this is
    not currently an issue.
    */
    public static boolean validateConsistentFaceOrientations(
        PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        int i;
        boolean test;

        if ( solid == null || solid.getPolygonsList() == null ||
             solid.getPolygonsList().size() == 0 ) {
            return true;
        }
        if ( numericContext == null ) {
            numericContext =
                PolyhedralBoundedSolidNumericPolicy.defaultContext();
        }
        test = true;
        for ( i = 0; i < solid.getPolygonsList().size(); i++ ) {
            _PolyhedralBoundedSolidFace face =
                solid.getPolygonsList().get(i);
            if ( !faceAgreesWithNeighbours(face, msg) ) {
                test = false;
            }
        }
        return test;
    }

    private static final double INVERTED_FACE_THRESHOLD = 0.5;

    private static boolean faceAgreesWithNeighbours(
        _PolyhedralBoundedSolidFace face, StringBuilder msg)
    {
        InfinitePlane planeF;
        Vector3Dd nF;
        int i;
        int j;
        int neighbourCount;
        int anomalousCount;
        double worstDot;

        if ( face == null || face.boundariesList == null ||
             face.boundariesList.size() == 0 ) {
            return true;
        }
        planeF = face.getContainingPlane();
        if ( planeF == null ) {
            return true;
        }
        nF = planeF.getNormal();
        if ( nF == null ) {
            return true;
        }
        neighbourCount = 0;
        anomalousCount = 0;
        worstDot = 1.0;
        for ( i = 0; i < face.boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop loop = face.boundariesList.get(i);
            if ( loop == null || loop.halfEdgesList == null ) {
                continue;
            }
            for ( j = 0; j < loop.halfEdgesList.size(); j++ ) {
                _PolyhedralBoundedSolidHalfEdge he =
                    loop.halfEdgesList.get(j);
                if ( he == null || he.mirrorHalfEdge() == null ||
                     he.mirrorHalfEdge().parentLoop == null ) {
                    continue;
                }
                _PolyhedralBoundedSolidFace neighbour =
                    he.mirrorHalfEdge().parentLoop.parentFace;
                if ( neighbour == null || neighbour == face ) {
                    continue;
                }
                InfinitePlane planeN = neighbour.getContainingPlane();
                if ( planeN == null ) {
                    continue;
                }
                Vector3Dd nN = planeN.getNormal();
                if ( nN == null ) {
                    continue;
                }
                neighbourCount++;
                double dot = nF.dotProduct(nN);
                if ( dot < worstDot ) {
                    worstDot = dot;
                }
                if ( dot < -INVERTED_FACE_THRESHOLD ) {
                    anomalousCount++;
                }
            }
        }
        // Flag only when EVERY neighbour disagrees strongly (consistent
        // inversion sign).  A single sharp dihedral is not enough.
        if ( neighbourCount >= 2 && anomalousCount == neighbourCount ) {
            if ( msg != null ) {
                msg.append("  - Face [").append(face.id)
                   .append("] is opposed to all ").append(neighbourCount)
                   .append(" neighbours (worst cos=")
                   .append(String.format("%.3f", worstDot))
                   .append(")\n");
            }
            return false;
        }
        return true;
    }

    private static int dominantCoordinateForFace(_PolyhedralBoundedSolidFace face)
    {
        Vector3Dd n = face.getContainingPlane().getNormal();
        if ( Math.abs(n.x()) >= Math.abs(n.y()) && Math.abs(n.x()) >= Math.abs(n.z()) ) {
            return 1;
        }
        if ( Math.abs(n.y()) >= Math.abs(n.x()) && Math.abs(n.y()) >= Math.abs(n.z()) ) {
            return 2;
        }
        return 3;
    }

    private static Vector2Dd projectPointTo2D(Vector3Dd in, int dominantCoordinate)
    {
        if ( dominantCoordinate == 1 ) {
            return new Vector2Dd(in.y(), in.z());
        }
        if ( dominantCoordinate == 2 ) {
            return new Vector2Dd(in.x(), in.z());
        }
        return new Vector2Dd(in.x(), in.y());
    }

    private static double orientation2D(Vector2Dd a, Vector2Dd b, Vector2Dd c)
    {
        return (b.x()-a.x())*(c.y()-a.y()) - (b.y()-a.y())*(c.x()-a.x());
    }

    private static boolean pointOnSegment2D(Vector2Dd p, Vector2Dd a, Vector2Dd b,
                                            double orientationTolerance,
                                            double linearTolerance)
    {
        if ( Math.abs(orientation2D(a, b, p)) > orientationTolerance ) {
            return false;
        }
        double minX = Math.min(a.x(), b.x()) - linearTolerance;
        double maxX = Math.max(a.x(), b.x()) + linearTolerance;
        double minY = Math.min(a.y(), b.y()) - linearTolerance;
        double maxY = Math.max(a.y(), b.y()) + linearTolerance;
        return p.x() >= minX && p.x() <= maxX && p.y() >= minY && p.y() <= maxY;
    }

    private static boolean segmentsIntersect2D(Vector2Dd a1, Vector2Dd a2,
                                               Vector2Dd b1, Vector2Dd b2,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext)
    {
        double o1 = orientation2D(a1, a2, b1);
        double o2 = orientation2D(a1, a2, b2);
        double o3 = orientation2D(b1, b2, a1);
        double o4 = orientation2D(b1, b2, a2);

        double orientationTolerance = PolyhedralBoundedSolidNumericPolicy
            .orientationTolerance2D(a1, a2, b1, numericContext);
        orientationTolerance = Math.max(orientationTolerance,
            PolyhedralBoundedSolidNumericPolicy
                .orientationTolerance2D(a1, a2, b2, numericContext));
        orientationTolerance = Math.max(orientationTolerance,
            PolyhedralBoundedSolidNumericPolicy
                .orientationTolerance2D(b1, b2, a1, numericContext));
        orientationTolerance = Math.max(orientationTolerance,
            PolyhedralBoundedSolidNumericPolicy
                .orientationTolerance2D(b1, b2, a2, numericContext));
        double linearTolerance =
            PolyhedralBoundedSolidNumericPolicy.linearTolerance2D(numericContext);

        boolean proper = (o1 > orientationTolerance && o2 < -orientationTolerance ||
                          o1 < -orientationTolerance && o2 > orientationTolerance) &&
                         (o3 > orientationTolerance && o4 < -orientationTolerance ||
                          o3 < -orientationTolerance && o4 > orientationTolerance);
        if ( proper ) {
            return true;
        }

        return pointOnSegment2D(b1, a1, a2, orientationTolerance, linearTolerance) ||
               pointOnSegment2D(b2, a1, a2, orientationTolerance, linearTolerance) ||
               pointOnSegment2D(a1, b1, b2, orientationTolerance, linearTolerance) ||
               pointOnSegment2D(a2, b1, b2, orientationTolerance, linearTolerance);
    }

    private static boolean loopHasSelfIntersection(_PolyhedralBoundedSolidFace face,
                                                   _PolyhedralBoundedSolidLoop loop,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
                                                   StringBuilder msg)
    {
        int n = loop.halfEdgesList.size();
        if ( n < 3 ) {
            msg.append("  - Face [").append(face.id)
               .append("] has a loop with fewer than 3 edges.\n");
            return true;
        }

        int dominantCoordinate = dominantCoordinateForFace(face);
        int i;
        int j;
        for ( i = 0; i < n; i++ ) {
            _PolyhedralBoundedSolidHalfEdge heA = loop.halfEdgesList.get(i);
            _PolyhedralBoundedSolidHalfEdge heANext = heA.next();
            if ( heANext == null ) {
                msg.append("  - Face [").append(face.id)
                   .append("] has a non-closed loop during strict validation.\n");
                return true;
            }
            Vector2Dd a1 = projectPointTo2D(heA.startingVertex.position, dominantCoordinate);
            Vector2Dd a2 = projectPointTo2D(heANext.startingVertex.position, dominantCoordinate);

            for ( j = i+1; j < n; j++ ) {
                if ( j == (i+1)%n || i == (j+1)%n ) {
                    continue;
                }
                _PolyhedralBoundedSolidHalfEdge heB = loop.halfEdgesList.get(j);
                _PolyhedralBoundedSolidHalfEdge heBNext = heB.next();
                if ( heBNext == null ) {
                    msg.append("  - Face [").append(face.id)
                       .append("] has a non-closed loop during strict validation.\n");
                    return true;
                }
                Vector2Dd b1 = projectPointTo2D(heB.startingVertex.position, dominantCoordinate);
                Vector2Dd b2 = projectPointTo2D(heBNext.startingVertex.position, dominantCoordinate);

                if ( segmentsIntersect2D(a1, a2, b1, b2, numericContext) ) {
                    msg.append("  - Face [").append(face.id)
                       .append("] has a self-intersecting loop.\n");
                    return true;
                }
            }
        }
        return false;
    }

    private static boolean loopsIntersect(_PolyhedralBoundedSolidFace face,
                                          _PolyhedralBoundedSolidLoop loopA,
                                          _PolyhedralBoundedSolidLoop loopB,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
                                          StringBuilder msg)
    {
        int dominantCoordinate = dominantCoordinateForFace(face);
        int i;
        int j;
        for ( i = 0; i < loopA.halfEdgesList.size(); i++ ) {
            _PolyhedralBoundedSolidHalfEdge heA = loopA.halfEdgesList.get(i);
            _PolyhedralBoundedSolidHalfEdge heANext = heA.next();
            if ( heANext == null ) {
                msg.append("  - Face [").append(face.id)
                   .append("] has a non-closed loop during strict validation.\n");
                return true;
            }
            Vector2Dd a1 = projectPointTo2D(heA.startingVertex.position, dominantCoordinate);
            Vector2Dd a2 = projectPointTo2D(heANext.startingVertex.position, dominantCoordinate);

            for ( j = 0; j < loopB.halfEdgesList.size(); j++ ) {
                _PolyhedralBoundedSolidHalfEdge heB = loopB.halfEdgesList.get(j);
                _PolyhedralBoundedSolidHalfEdge heBNext = heB.next();
                if ( heBNext == null ) {
                    msg.append("  - Face [").append(face.id)
                       .append("] has a non-closed loop during strict validation.\n");
                    return true;
                }
                Vector2Dd b1 = projectPointTo2D(heB.startingVertex.position, dominantCoordinate);
                Vector2Dd b2 = projectPointTo2D(heBNext.startingVertex.position, dominantCoordinate);
                if ( segmentsIntersect2D(a1, a2, b1, b2, numericContext) ) {
                    msg.append("  - Face [").append(face.id)
                       .append("] has intersecting loops.\n");
                    return true;
                }
            }
        }
        return false;
    }

    /**
    Validates that loops can act as proper planar face boundaries, consistent
    with the face/loop organization of [MANT1988].10.2.1 and the planar
    geometric reasoning of chapter [MANT1988].13.
    */
    public static boolean validateLoopsStrict(PolyhedralBoundedSolid solid, StringBuilder msg)
    {
        return validateLoopsStrict(solid,
            PolyhedralBoundedSolidNumericPolicy.forSolid(solid), msg);
    }

    /**
    Enforces strict geometric loop consistency for the planar polygons assumed
    by [MANT1988].10.2.1 and manipulated by the geometric tools of chapter
    [MANT1988].13.
    */
    public static boolean validateLoopsStrict(PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        int i;
        int j;
        int k;
        for ( i = 0; i < solid.getPolygonsList().size(); i++ ) {
            _PolyhedralBoundedSolidFace face = solid.getPolygonsList().get(i);
            if ( face.getContainingPlane() == null ) {
                msg.append("  - Face [").append(face.id)
                   .append("] has no containing plane for strict checks.\n");
                return false;
            }

            for ( j = 0; j < face.boundariesList.size(); j++ ) {
                _PolyhedralBoundedSolidLoop loop = face.boundariesList.get(j);
                if ( loopHasSelfIntersection(face, loop, numericContext, msg) ) {
                    return false;
                }
            }
            for ( j = 0; j < face.boundariesList.size(); j++ ) {
                for ( k = j+1; k < face.boundariesList.size(); k++ ) {
                    if ( loopsIntersect(face, face.boundariesList.get(j),
                                        face.boundariesList.get(k),
                                        numericContext, msg) ) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    private static boolean facesAreCoplanar(_PolyhedralBoundedSolidFace faceA,
                                            _PolyhedralBoundedSolidFace faceB,
        InfinitePlane planeA, InfinitePlane planeB,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext)
    {
        if ( planeA == null || planeB == null ) {
            return false;
        }

        Vector3Dd nA = planeA.getNormal().multiply(1.0);
        Vector3Dd nB = planeB.getNormal().multiply(1.0);
        nA = nA.normalized();
        nB = nB.normalized();
        if ( Math.abs(Math.abs(nA.dotProduct(nB)) - 1.0) >
             numericContext.coplanarDotTolerance() ) {
            return false;
        }

        for ( int i = 0; i < faceA.boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop loop = faceA.boundariesList.get(i);
            if ( loop.halfEdgesList.size() > 0 ) {
                Vector3Dd p = loop.halfEdgesList.get(0).startingVertex.position;
                return Math.abs(planeB.pointDistance(p)) <=
                    numericContext.bigEpsilon();
            }
        }
        return false;
    }

    private static boolean segmentSharesEndpoint(_PolyhedralBoundedSolidHalfEdge a,
                                                 _PolyhedralBoundedSolidHalfEdge b)
    {
        _PolyhedralBoundedSolidHalfEdge an = a.next();
        _PolyhedralBoundedSolidHalfEdge bn = b.next();
        if ( an == null || bn == null ) {
            return false;
        }
        _PolyhedralBoundedSolidVertex a0 = a.startingVertex;
        _PolyhedralBoundedSolidVertex a1 = an.startingVertex;
        _PolyhedralBoundedSolidVertex b0 = b.startingVertex;
        _PolyhedralBoundedSolidVertex b1 = bn.startingVertex;
        return a0 == b0 || a0 == b1 || a1 == b0 || a1 == b1;
    }

    private static boolean vertexStrictlyInsideFace(_PolyhedralBoundedSolidVertex v,
                                                    _PolyhedralBoundedSolidFace face,
        PolyhedralBoundedSolidFaceGeometry faceGeometry,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext)
    {
        InfinitePlane plane = faceGeometry.getPlane();
        if ( plane.doContainmentTest(v.position,
            numericContext.bigEpsilon()) != Geometry.LIMIT ) {
            return false;
        }
        if ( faceGeometry.projectionExcludes(v.position,
                 numericContext.bigEpsilon()) ) {
            return false;
        }
        return face.testPointInside(v.position, numericContext.bigEpsilon(),
            plane) == Geometry.INSIDE;
    }

    private static boolean edgePiercesFaceInterior(_PolyhedralBoundedSolidHalfEdge he,
                                                   _PolyhedralBoundedSolidFace face,
        PolyhedralBoundedSolidFaceGeometry faceGeometry,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext)
    {
        _PolyhedralBoundedSolidHalfEdge next = he.next();
        InfinitePlane plane = faceGeometry.getPlane();
        if ( next == null || plane == null ) {
            return false;
        }

        Vector3Dd p0 = he.startingVertex.position;
        Vector3Dd p1 = next.startingVertex.position;
        double d0 = plane.pointDistance(p0);
        double d1 = plane.pointDistance(p1);

        if ( Math.abs(d0) <= numericContext.bigEpsilon() &&
             Math.abs(d1) <= numericContext.bigEpsilon() ) {
            return false;
        }
        if ( d0*d1 > 0 ) {
            return false;
        }

        double denom = d0 - d1;
        if ( PolyhedralBoundedSolidNumericPolicy.isZero(denom, numericContext) ) {
            return false;
        }
        double t = d0 / denom;
        if ( !PolyhedralBoundedSolidNumericPolicy
            .unitIntervalContainsStrictly(t, numericContext) ) {
            return false;
        }

        Vector3Dd p = p0.add(p1.subtract(p0).multiply(t));
        if ( faceGeometry.projectionExcludes(p, numericContext.bigEpsilon()) ) {
            return false;
        }
        return face.testPointInside(p, numericContext.bigEpsilon(), plane) ==
            Geometry.INSIDE;
    }

    private static boolean facesHaveImproperIntersection(_PolyhedralBoundedSolidFace faceA,
                                                         _PolyhedralBoundedSolidFace faceB,
        PolyhedralBoundedSolidFaceGeometry geometryA,
        PolyhedralBoundedSolidFaceGeometry geometryB,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
                                                         StringBuilder msg)
    {
        int i;
        int j;
        int k;
        _PolyhedralBoundedSolidHalfEdge he;

        for ( i = 0; i < faceA.boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop loop = faceA.boundariesList.get(i);
            for ( j = 0; j < loop.halfEdgesList.size(); j++ ) {
                he = loop.halfEdgesList.get(j);
                if ( geometryB.getPlane() != null &&
                     vertexStrictlyInsideFace(he.startingVertex, faceB,
                                              geometryB, numericContext) ) {
                    msg.append("  - Faces [").append(faceA.id).append("] and [")
                       .append(faceB.id).append("] intersect improperly.\n");
                    return true;
                }
                if ( edgePiercesFaceInterior(he, faceB, geometryB,
                                             numericContext) ) {
                    msg.append("  - Faces [").append(faceA.id).append("] and [")
                       .append(faceB.id).append("] intersect improperly.\n");
                    return true;
                }
            }
        }

        for ( i = 0; i < faceB.boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop loop = faceB.boundariesList.get(i);
            for ( j = 0; j < loop.halfEdgesList.size(); j++ ) {
                he = loop.halfEdgesList.get(j);
                if ( geometryA.getPlane() != null &&
                     vertexStrictlyInsideFace(he.startingVertex, faceA,
                                              geometryA, numericContext) ) {
                    msg.append("  - Faces [").append(faceA.id).append("] and [")
                       .append(faceB.id).append("] intersect improperly.\n");
                    return true;
                }
                if ( edgePiercesFaceInterior(he, faceA, geometryA,
                                             numericContext) ) {
                    msg.append("  - Faces [").append(faceA.id).append("] and [")
                       .append(faceB.id).append("] intersect improperly.\n");
                    return true;
                }
            }
        }

        if ( facesAreCoplanar(faceA, faceB, geometryA.getPlane(),
                 geometryB.getPlane(), numericContext) ) {
            int dominantCoordinate = dominantCoordinateForFace(faceA);
            for ( i = 0; i < faceA.boundariesList.size(); i++ ) {
                _PolyhedralBoundedSolidLoop loopA = faceA.boundariesList.get(i);
                for ( j = 0; j < loopA.halfEdgesList.size(); j++ ) {
                    _PolyhedralBoundedSolidHalfEdge heA = loopA.halfEdgesList.get(j);
                    _PolyhedralBoundedSolidHalfEdge heANext = heA.next();
                    if ( heANext == null ) {
                        continue;
                    }
                    Vector2Dd a1 = projectPointTo2D(heA.startingVertex.position, dominantCoordinate);
                    Vector2Dd a2 = projectPointTo2D(heANext.startingVertex.position, dominantCoordinate);

                    for ( k = 0; k < faceB.boundariesList.size(); k++ ) {
                        _PolyhedralBoundedSolidLoop loopB = faceB.boundariesList.get(k);
                        for ( int m = 0; m < loopB.halfEdgesList.size(); m++ ) {
                            _PolyhedralBoundedSolidHalfEdge heB = loopB.halfEdgesList.get(m);
                            _PolyhedralBoundedSolidHalfEdge heBNext = heB.next();
                            if ( heBNext == null ) {
                                continue;
                            }
                            if ( heA.parentEdge == heB.parentEdge ) {
                                continue;
                            }
                            Vector2Dd b1 = projectPointTo2D(heB.startingVertex.position, dominantCoordinate);
                            Vector2Dd b2 = projectPointTo2D(heBNext.startingVertex.position, dominantCoordinate);
                            if ( segmentsIntersect2D(a1, a2, b1, b2, numericContext) &&
                                 !segmentSharesEndpoint(heA, heB) ) {
                                msg.append("  - Faces [").append(faceA.id).append("] and [")
                                   .append(faceB.id).append("] intersect improperly.\n");
                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    /**
    Validates the no-self-intersection requirement stated for valid boundary
    models in [MANT1988].15.2, criterion 3.
    */
    public static boolean validateFaceIntersectionsStrict(
        PolyhedralBoundedSolid solid, StringBuilder msg)
    {
        return validateFaceIntersectionsStrict(solid,
            PolyhedralBoundedSolidNumericPolicy.forSolid(solid), msg);
    }

    /**
    Applies strict face/face intersection checks to preserve the validity
    condition from [MANT1988].15.2, criterion 3, using predicates from chapter
    [MANT1988].13.
    */
    public static boolean validateFaceIntersectionsStrict(
        PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        int i;
        int j;
        int n = solid.getPolygonsList().size();
        _PolyhedralBoundedSolidFace[] faces = new _PolyhedralBoundedSolidFace[n];
        PolyhedralBoundedSolidFaceGeometry[] geometry;
        double[] margin = new double[n];
        double coplanarSlack;

        // Stage G (Fable5.2Analysis.md): planes are computed once per pass
        // instead of once per face pair and per point, and face pairs whose
        // grown boxes are apart are skipped. Every improper intersection has
        // a witness point that lies in both grown boxes: a vertex of one face
        // inside the other, the point where an edge pierces a face, or a
        // crossing of two coplanar edges. For the last one the faces may be
        // tilted by up to acos(1 - coplanarDotTolerance), which coplanarSlack
        // bounds over the whole solid.
        geometry = PolyhedralBoundedSolidFaceGeometry.of(solid);
        coplanarSlack = 2.0 * Math.sqrt(2.0 *
            numericContext.coplanarDotTolerance()) * solidDiagonal(solid);
        for ( i = 0; i < n; i++ ) {
            faces[i] = solid.getPolygonsList().get(i);
            margin[i] = geometry[i].containmentMargin(
                numericContext.bigEpsilon());
        }
        for ( i = 0; i < n; i++ ) {
            for ( j = i+1; j < n; j++ ) {
                if ( geometry[i].boxSeparatedFrom(geometry[j],
                         margin[i] + margin[j] + coplanarSlack) ) {
                    continue;
                }
                if ( facesHaveImproperIntersection(faces[i], faces[j],
                         geometry[i], geometry[j], numericContext, msg) ) {
                    return false;
                }
            }
        }
        return true;
    }

    private static double solidDiagonal(PolyhedralBoundedSolid solid)
    {
        double[] box = solid.getMinMax();
        return Math.sqrt((box[3] - box[0]) * (box[3] - box[0]) +
            (box[4] - box[1]) * (box[4] - box[1]) +
            (box[5] - box[2]) * (box[5] - box[2]));
    }

    /**
    Area below which a face is collapsed: VSDK.EPSILON scaled by the square
    of the model scale, so that it equals EPSILON for models of unit size and
    is invariant under a uniform change of units.
    */
    public static double collapsedFaceAreaThreshold(
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext)
    {
        return VSDK.EPSILON * numericContext.modelScale() *
            numericContext.modelScale();
    }

    /**
    Area of a face: half the length of the Newell vector summed over all its
    loops, so inner loops (holes, oriented the other way) are subtracted.
    */
    public static double faceArea(_PolyhedralBoundedSolidFace face)
    {
        double nx = 0.0;
        double ny = 0.0;
        double nz = 0.0;
        int i;

        for ( i = 0; i < face.boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop loop = face.boundariesList.get(i);
            _PolyhedralBoundedSolidHalfEdge start;
            _PolyhedralBoundedSolidHalfEdge he;
            if ( loop == null || loop.boundaryStartHalfEdge == null ) {
                continue;
            }
            start = loop.boundaryStartHalfEdge;
            he = start;
            do {
                _PolyhedralBoundedSolidHalfEdge next = he.next();
                if ( next == null ) {
                    break;
                }
                Vector3Dd p = he.startingVertex.position;
                Vector3Dd q = next.startingVertex.position;
                nx += (p.y() - q.y()) * (p.z() + q.z());
                ny += (p.z() - q.z()) * (p.x() + q.x());
                nz += (p.x() - q.x()) * (p.y() + q.y());
                he = next;
            } while ( he != start );
        }
        return 0.5 * Math.sqrt(nx * nx + ny * ny + nz * nz);
    }

    /**
    Rejects collapsed laminae. A lamina is a shell made of exactly two faces
    (which then share all their edges, back to back); it has zero volume and
    is a valid result. It is collapsed when its area is below
    collapsedFaceAreaThreshold. Small faces of shells that enclose volume
    (slivers of a tessellation) are not judged by this rule.
    */
    public static boolean validateNoCollapsedLaminae(
        PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        double threshold = collapsedFaceAreaThreshold(numericContext);
        java.util.IdentityHashMap<_PolyhedralBoundedSolidFace, Integer> shellOf =
            new java.util.IdentityHashMap<>();
        java.util.ArrayList<java.util.ArrayList<_PolyhedralBoundedSolidFace>>
            shells = new java.util.ArrayList<>();
        boolean ok = true;
        int i;

        for ( i = 0; i < solid.getPolygonsList().size(); i++ ) {
            _PolyhedralBoundedSolidFace seed = solid.getPolygonsList().get(i);
            if ( shellOf.containsKey(seed) ) {
                continue;
            }
            java.util.ArrayList<_PolyhedralBoundedSolidFace> shell =
                new java.util.ArrayList<>();
            java.util.ArrayDeque<_PolyhedralBoundedSolidFace> pending =
                new java.util.ArrayDeque<>();
            shellOf.put(seed, shells.size());
            pending.add(seed);
            while ( !pending.isEmpty() ) {
                _PolyhedralBoundedSolidFace face = pending.poll();
                shell.add(face);
                for ( int j = 0; j < face.boundariesList.size(); j++ ) {
                    _PolyhedralBoundedSolidLoop loop =
                        face.boundariesList.get(j);
                    for ( int k = 0; k < loop.halfEdgesList.size(); k++ ) {
                        _PolyhedralBoundedSolidHalfEdge he =
                            loop.halfEdgesList.get(k);
                        _PolyhedralBoundedSolidHalfEdge mate =
                            he.parentEdge == null ? null :
                            he.mirrorHalfEdge();
                        if ( mate == null || mate.parentLoop == null ) {
                            continue;
                        }
                        _PolyhedralBoundedSolidFace other =
                            mate.parentLoop.parentFace;
                        if ( other != null && !shellOf.containsKey(other) ) {
                            shellOf.put(other, shells.size());
                            pending.add(other);
                        }
                    }
                }
            }
            shells.add(shell);
        }

        for ( i = 0; i < shells.size(); i++ ) {
            java.util.ArrayList<_PolyhedralBoundedSolidFace> shell =
                shells.get(i);
            if ( shell.size() != 2 ) {
                continue;
            }
            double area = faceArea(shell.get(0));
            if ( area < threshold ) {
                msg.append("  - Lamina of faces [").append(shell.get(0).id)
                   .append("] and [").append(shell.get(1).id)
                   .append("] is collapsed: area ").append(area)
                   .append(" < ").append(threshold).append(".\n");
                ok = false;
            }
        }
        return ok;
    }

    /**
    Checks that no two distinct vertices in {@code solid} are geometrically
    coincident (distance ≤ {@code context.bigEpsilon()}).  Coincident vertices
    in a boolean operand can cause the null-edge connector of [MANT1988].15.7
    to produce "loose" half-edges and an empty result.
    @param solid solid to inspect.
    @param context tolerance context; use {@code forSolids(a,b)} for boolean
    operand pairs so both solids share the same scale.
    @param msg collects one line per coincident pair found.
    @return true when no coincident vertices are found.
    */
    public static boolean validateNoCoincidentVertices(
        PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext context,
        StringBuilder msg)
    {
        int i;
        int j;
        boolean ok;
        _PolyhedralBoundedSolidVertex vi;
        _PolyhedralBoundedSolidVertex vj;

        ok = true;
        for ( i = 0; i < solid.getVerticesList().size(); i++ ) {
            vi = solid.getVerticesList().get(i);
            if ( vi == null || vi.position == null ) {
                continue;
            }
            for ( j = i + 1; j < solid.getVerticesList().size(); j++ ) {
                vj = solid.getVerticesList().get(j);
                if ( vj == null || vj.position == null ) {
                    continue;
                }
                if ( PolyhedralBoundedSolidNumericPolicy.pointsCoincident(
                         vi.position, vj.position, context) ) {
                    msg.append("  coincident vertices: v").append(vi.id)
                       .append(" and v").append(vj.id)
                       .append(" at ").append(vi.position).append("\n");
                    ok = false;
                }
            }
        }
        return ok;
    }

    /**
    Checks that {@code solid.getMaxFaceId()} and {@code solid.getMaxVertexId()}
    are consistent with the IDs actually present in the face and vertex lists.
    Specifically: no face id exceeds maxFaceId, no vertex id exceeds maxVertexId,
    and no two faces (or vertices) share the same id.
    @param solid solid to inspect.
    @param msg collects one line per violation found.
    @return true when IDs are unique and the stored maxima cover all elements.
    */
    public static boolean validateUniqueFaceAndVertexIds(
        PolyhedralBoundedSolid solid,
        StringBuilder msg)
    {
        boolean ok;
        int i;
        int j;
        _PolyhedralBoundedSolidFace fi;
        _PolyhedralBoundedSolidFace fj;
        _PolyhedralBoundedSolidVertex vi;
        _PolyhedralBoundedSolidVertex vj;

        ok = true;

        for ( i = 0; i < solid.getPolygonsList().size(); i++ ) {
            fi = solid.getPolygonsList().get(i);
            if ( fi == null ) {
                continue;
            }
            if ( fi.id > solid.getMaxFaceId() ) {
                msg.append("  face id ").append(fi.id)
                   .append(" exceeds maxFaceId=").append(solid.getMaxFaceId())
                   .append("\n");
                ok = false;
            }
            for ( j = i + 1; j < solid.getPolygonsList().size(); j++ ) {
                fj = solid.getPolygonsList().get(j);
                if ( fj != null && fi.id == fj.id ) {
                    msg.append("  duplicate face id=").append(fi.id).append("\n");
                    ok = false;
                }
            }
        }

        for ( i = 0; i < solid.getVerticesList().size(); i++ ) {
            vi = solid.getVerticesList().get(i);
            if ( vi == null ) {
                continue;
            }
            if ( vi.id > solid.getMaxVertexId() ) {
                msg.append("  vertex id ").append(vi.id)
                   .append(" exceeds maxVertexId=").append(solid.getMaxVertexId())
                   .append("\n");
                ok = false;
            }
            for ( j = i + 1; j < solid.getVerticesList().size(); j++ ) {
                vj = solid.getVerticesList().get(j);
                if ( vj != null && vi.id == vj.id ) {
                    msg.append("  duplicate vertex id=").append(vi.id).append("\n");
                    ok = false;
                }
            }
        }

        return ok;
    }
}
