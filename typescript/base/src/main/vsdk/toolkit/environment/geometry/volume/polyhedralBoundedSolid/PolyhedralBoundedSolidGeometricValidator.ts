//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { Double } from "../../../../../../java/lang/Double.js";
import type { StringBuilder } from "../../../../../../java/lang/StringBuilder.js";
import { VSDK } from "../../../../common/VSDK.js";
import { Vector2Dd } from "../../../../common/linealAlgebra/Vector2Dd.js";
import type { Vector3Dd } from "../../../../common/linealAlgebra/Vector3Dd.js";
import { Geometry } from "../../Geometry.js";
import { InfinitePlane } from "../../surface/InfinitePlane.js";
import type { _PolyhedralBoundedSolidFace } from "./nodes/_PolyhedralBoundedSolidFace.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./nodes/_PolyhedralBoundedSolidHalfEdge.js";
import type { _PolyhedralBoundedSolidLoop } from "./nodes/_PolyhedralBoundedSolidLoop.js";
import type { _PolyhedralBoundedSolidVertex } from "./nodes/_PolyhedralBoundedSolidVertex.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidFaceGeometry } from "./PolyhedralBoundedSolidFaceGeometry.js";
import { PolyhedralBoundedSolidNumericPolicy, type ToleranceContext } from "./PolyhedralBoundedSolidNumericPolicy.js";

/**
Geometric validation helpers for polyhedral B-Reps, centered on the planar
face model of [MANT1988].10.2.1 and the geometric primitives developed in
chapter [MANT1988].13.
*/
export class PolyhedralBoundedSolidGeometricValidator {
    private constructor() {}

    /**
    Checks whether a set of face vertices can define the planar polygon
    required by [MANT1988].10.2.1 and by the face-equation procedure of
    [MANT1988].13.1.

    Java overload with a tolerance context: implements the coplanarity
    precondition needed before applying the face equation ideas of
    [MANT1988].13.1 to a polyhedral face from [MANT1988].10.2.1.
    */
    public static validateFacePointsAreCoplanar(
        points: readonly Vector3Dd[] | null,
        numericContext?: ToleranceContext | null,
    ): boolean {
        if (numericContext === undefined) {
            return PolyhedralBoundedSolidGeometricValidator.validateFacePointsAreCoplanar(
                points,
                PolyhedralBoundedSolidNumericPolicy.forPoints(points),
            );
        }
        if (points === null || points.length < 3) {
            return false;
        }
        if (numericContext === null) {
            numericContext = PolyhedralBoundedSolidNumericPolicy.defaultContext();
        }

        let p0: Vector3Dd;
        let p1: Vector3Dd;
        let p2: Vector3Dd;
        p0 = points[0]!;
        let foundSeparatedPair = false;

        let i: number;
        for (i = 1; i < points.length; i++) {
            p1 = points[i]!;
            if (PolyhedralBoundedSolidNumericPolicy.pointsSeparated(p0, p1, numericContext)) {
                foundSeparatedPair = true;
                break;
            }
        }
        if (!foundSeparatedPair) {
            return false;
        }

        let a: Vector3Dd;
        let b: Vector3Dd;
        let n: Vector3Dd;
        let aDotB: number;
        let facePlane: InfinitePlane | null = null;
        let j: number;
        let k: number;

        // The plane is the one of the last pair (i, j), in ascending
        // order, whose first admissible third point k is not collinear
        // with it. The pairs are visited from the last one backwards and
        // the search stops at the first that gives a plane: the same
        // plane as visiting every pair and keeping the last one, which
        // took a time cubic in the number of points (performance.md).
        for (i = points.length - 1; i >= 0 && facePlane === null; i--) {
            for (j = points.length - 1; j >= 0 && facePlane === null; j--) {
                for (k = 0; k < points.length; k++) {
                    if (i === j || i === k || j === k) {
                        continue;
                    }
                    p0 = points[i]!;
                    p1 = points[j]!;
                    p2 = points[k]!;
                    if (
                        PolyhedralBoundedSolidNumericPolicy.pointsSeparated(p0, p2, numericContext) &&
                        PolyhedralBoundedSolidNumericPolicy.pointsSeparated(p1, p2, numericContext)
                    ) {
                        a = p2.subtract(p0);
                        b = p1.subtract(p0);
                        a = a.normalized();
                        b = b.normalized();
                        aDotB = Math.abs(a.dotProduct(b));
                        if (aDotB < 1.0 - numericContext.unitVectorTolerance()) {
                            n = a.crossProduct(b);
                            n = n.normalized();
                            facePlane = new InfinitePlane(n, p0);
                        }
                        break;
                    }
                }
            }
        }

        if (facePlane === null) {
            return false;
        }

        for (i = 1; i < points.length; i++) {
            p0 = points[i]!;
            if (facePlane.doContainmentTest(p0, numericContext.epsilon()) !== Geometry.LIMIT) {
                return false;
            }
        }

        return true;
    }

    public static extractPointsFromFace(face: _PolyhedralBoundedSolidFace): Vector3Dd[] | null {
        let test = true;
        const points: Vector3Dd[] = [];
        let j: number;

        for (j = 0; j < face.boundariesList.size(); j++) {
            let he: _PolyhedralBoundedSolidHalfEdge | null;

            const loop = face.boundariesList.get(j)!;
            he = loop.boundaryStartHalfEdge;
            if (he === null) {
                test = false;
                break;
            }
            const heStart = he;
            do {
                he = he!.next();
                if (he === null) {
                    test = false;
                    break;
                }
                points.push(he.startingVertex.position);
            } while (he !== heStart);
        }

        if (!test) {
            return null;
        }
        return points;
    }

    /**
    Validates that a face instance matches the planar-face assumption from
    [MANT1988].10.2.1.

    Java overload with a tolerance context: validates the face-planarity
    invariant required before evaluating face equations as in
    [MANT1988].13.1.
    */
    public static validateFaceIsPlanar(
        face: _PolyhedralBoundedSolidFace,
        numericContext?: ToleranceContext | null,
    ): boolean {
        if (numericContext === undefined) {
            return PolyhedralBoundedSolidGeometricValidator.validateFaceIsPlanar(
                face,
                PolyhedralBoundedSolidNumericPolicy.forFace(face),
            );
        }
        const points = PolyhedralBoundedSolidGeometricValidator.extractPointsFromFace(face);
        return (
            points !== null &&
            PolyhedralBoundedSolidGeometricValidator.validateFacePointsAreCoplanar(points, numericContext)
        );
    }

    /**
    Rebuilds the plane equations expected for face nodes in [MANT1988].10.2.1
    by first checking planarity and then evaluating planes in the spirit of
    [MANT1988].13.1.
    */
    public static validateAllFacesPlanarityAndPlanes(solid: PolyhedralBoundedSolid, msg: StringBuilder): boolean;
    /**
    Applies the face-planarity and face-equation checks corresponding to
    [MANT1988].10.2.1 and [MANT1988].13.1 to every face of the solid.
    */
    public static validateAllFacesPlanarityAndPlanes(
        solid: PolyhedralBoundedSolid,
        numericContext: ToleranceContext | null,
        msg: StringBuilder,
    ): boolean;
    public static validateAllFacesPlanarityAndPlanes(
        solid: PolyhedralBoundedSolid,
        numericContextOrMsg: ToleranceContext | StringBuilder | null,
        maybeMsg?: StringBuilder,
    ): boolean {
        if (maybeMsg === undefined) {
            return PolyhedralBoundedSolidGeometricValidator.validateAllFacesPlanarityAndPlanes(
                solid,
                PolyhedralBoundedSolidNumericPolicy.forSolid(solid),
                numericContextOrMsg as StringBuilder,
            );
        }
        const numericContext = numericContextOrMsg as ToleranceContext | null;
        const msg = maybeMsg;
        let i: number;
        let test = true;
        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const face = solid.getPolygonsList().get(i)!;
            if (PolyhedralBoundedSolidGeometricValidator.validateFaceIsPlanar(face, numericContext)) {
                if (face.getContainingPlane() === null) {
                    msg.append("  - Face [")
                        .append(String(face.id))
                        .append("] was not able to compute containing plane\n");
                    test = false;
                }
            } else {
                msg.append("  - Face [").append(String(face.id)).append("] is not coplanar\n");
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
    public static validateConsistentFaceOrientations(
        solid: PolyhedralBoundedSolid | null,
        msg: StringBuilder | null,
    ): boolean;
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
    public static validateConsistentFaceOrientations(
        solid: PolyhedralBoundedSolid | null,
        numericContext: ToleranceContext | null,
        msg: StringBuilder | null,
    ): boolean;
    public static validateConsistentFaceOrientations(
        solid: PolyhedralBoundedSolid | null,
        numericContextOrMsg: ToleranceContext | StringBuilder | null,
        maybeMsg?: StringBuilder | null,
    ): boolean {
        if (maybeMsg === undefined) {
            return PolyhedralBoundedSolidGeometricValidator.validateConsistentFaceOrientations(
                solid,
                PolyhedralBoundedSolidNumericPolicy.forSolid(solid),
                numericContextOrMsg as StringBuilder | null,
            );
        }
        let numericContext = numericContextOrMsg as ToleranceContext | null;
        const msg = maybeMsg;
        let i: number;
        let test: boolean;

        if (solid === null || solid.getPolygonsList() === null || solid.getPolygonsList().size() === 0) {
            return true;
        }
        if (numericContext === null) {
            numericContext = PolyhedralBoundedSolidNumericPolicy.defaultContext();
        }
        test = true;
        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const face = solid.getPolygonsList().get(i)!;
            if (!PolyhedralBoundedSolidGeometricValidator.faceAgreesWithNeighbours(face, msg)) {
                test = false;
            }
        }
        return test;
    }

    private static readonly INVERTED_FACE_THRESHOLD = 0.5;

    private static faceAgreesWithNeighbours(
        face: _PolyhedralBoundedSolidFace | null,
        msg: StringBuilder | null,
    ): boolean {
        let i: number;
        let j: number;
        let neighbourCount: number;
        let anomalousCount: number;
        let worstDot: number;

        if (face === null || face.boundariesList === null || face.boundariesList.size() === 0) {
            return true;
        }
        const planeF = face.getContainingPlane();
        if (planeF === null) {
            return true;
        }
        const nF = planeF.getNormal();
        if (nF === null) {
            return true;
        }
        neighbourCount = 0;
        anomalousCount = 0;
        worstDot = 1.0;
        for (i = 0; i < face.boundariesList.size(); i++) {
            const loop: _PolyhedralBoundedSolidLoop | null = face.boundariesList.get(i);
            if (loop === null || loop.halfEdgesList === null) {
                continue;
            }
            for (j = 0; j < loop.halfEdgesList.size(); j++) {
                const he: _PolyhedralBoundedSolidHalfEdge | null = loop.halfEdgesList.get(j);
                if (he === null || he.mirrorHalfEdge() === null || he.mirrorHalfEdge()!.parentLoop === null) {
                    continue;
                }
                const neighbour: _PolyhedralBoundedSolidFace | null = he.mirrorHalfEdge()!.parentLoop.parentFace;
                if (neighbour === null || neighbour === face) {
                    continue;
                }
                const planeN = neighbour.getContainingPlane();
                if (planeN === null) {
                    continue;
                }
                const nN = planeN.getNormal();
                if (nN === null) {
                    continue;
                }
                neighbourCount++;
                const dot = nF.dotProduct(nN);
                if (dot < worstDot) {
                    worstDot = dot;
                }
                if (dot < -PolyhedralBoundedSolidGeometricValidator.INVERTED_FACE_THRESHOLD) {
                    anomalousCount++;
                }
            }
        }
        // Flag only when EVERY neighbour disagrees strongly (consistent
        // inversion sign).  A single sharp dihedral is not enough.
        if (neighbourCount >= 2 && anomalousCount === neighbourCount) {
            if (msg !== null) {
                msg.append("  - Face [")
                    .append(String(face.id))
                    .append("] is opposed to all ")
                    .append(String(neighbourCount))
                    .append(" neighbours (worst cos=")
                    .append(worstDot.toFixed(3))
                    .append(")\n");
            }
            return false;
        }
        return true;
    }

    private static dominantCoordinateForFace(face: _PolyhedralBoundedSolidFace): number {
        const n = face.getContainingPlane()!.getNormal();
        if (Math.abs(n.x()) >= Math.abs(n.y()) && Math.abs(n.x()) >= Math.abs(n.z())) {
            return 1;
        }
        if (Math.abs(n.y()) >= Math.abs(n.x()) && Math.abs(n.y()) >= Math.abs(n.z())) {
            return 2;
        }
        return 3;
    }

    private static projectPointTo2D(input: Vector3Dd, dominantCoordinate: number): Vector2Dd {
        if (dominantCoordinate === 1) {
            return new Vector2Dd(input.y(), input.z());
        }
        if (dominantCoordinate === 2) {
            return new Vector2Dd(input.x(), input.z());
        }
        return new Vector2Dd(input.x(), input.y());
    }

    private static orientation2D(a: Vector2Dd, b: Vector2Dd, c: Vector2Dd): number {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    private static pointOnSegment2D(
        p: Vector2Dd,
        a: Vector2Dd,
        b: Vector2Dd,
        orientationTolerance: number,
        linearTolerance: number,
    ): boolean {
        if (Math.abs(PolyhedralBoundedSolidGeometricValidator.orientation2D(a, b, p)) > orientationTolerance) {
            return false;
        }
        const minX = Math.min(a.x, b.x) - linearTolerance;
        const maxX = Math.max(a.x, b.x) + linearTolerance;
        const minY = Math.min(a.y, b.y) - linearTolerance;
        const maxY = Math.max(a.y, b.y) + linearTolerance;
        return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
    }

    private static segmentsIntersect2D(
        a1: Vector2Dd,
        a2: Vector2Dd,
        b1: Vector2Dd,
        b2: Vector2Dd,
        numericContext: ToleranceContext,
    ): boolean {
        const o1 = PolyhedralBoundedSolidGeometricValidator.orientation2D(a1, a2, b1);
        const o2 = PolyhedralBoundedSolidGeometricValidator.orientation2D(a1, a2, b2);
        const o3 = PolyhedralBoundedSolidGeometricValidator.orientation2D(b1, b2, a1);
        const o4 = PolyhedralBoundedSolidGeometricValidator.orientation2D(b1, b2, a2);

        let orientationTolerance = PolyhedralBoundedSolidNumericPolicy.orientationTolerance2D(
            a1,
            a2,
            b1,
            numericContext,
        );
        orientationTolerance = Math.max(
            orientationTolerance,
            PolyhedralBoundedSolidNumericPolicy.orientationTolerance2D(a1, a2, b2, numericContext),
        );
        orientationTolerance = Math.max(
            orientationTolerance,
            PolyhedralBoundedSolidNumericPolicy.orientationTolerance2D(b1, b2, a1, numericContext),
        );
        orientationTolerance = Math.max(
            orientationTolerance,
            PolyhedralBoundedSolidNumericPolicy.orientationTolerance2D(b1, b2, a2, numericContext),
        );
        const linearTolerance = PolyhedralBoundedSolidNumericPolicy.linearTolerance2D(numericContext);

        const proper =
            ((o1 > orientationTolerance && o2 < -orientationTolerance) ||
                (o1 < -orientationTolerance && o2 > orientationTolerance)) &&
            ((o3 > orientationTolerance && o4 < -orientationTolerance) ||
                (o3 < -orientationTolerance && o4 > orientationTolerance));
        if (proper) {
            return true;
        }

        return (
            PolyhedralBoundedSolidGeometricValidator.pointOnSegment2D(
                b1,
                a1,
                a2,
                orientationTolerance,
                linearTolerance,
            ) ||
            PolyhedralBoundedSolidGeometricValidator.pointOnSegment2D(
                b2,
                a1,
                a2,
                orientationTolerance,
                linearTolerance,
            ) ||
            PolyhedralBoundedSolidGeometricValidator.pointOnSegment2D(
                a1,
                b1,
                b2,
                orientationTolerance,
                linearTolerance,
            ) ||
            PolyhedralBoundedSolidGeometricValidator.pointOnSegment2D(a2, b1, b2, orientationTolerance, linearTolerance)
        );
    }

    private static loopHasSelfIntersection(
        face: _PolyhedralBoundedSolidFace,
        loop: _PolyhedralBoundedSolidLoop,
        numericContext: ToleranceContext,
        msg: StringBuilder,
    ): boolean {
        const n = loop.halfEdgesList.size();
        if (n < 3) {
            msg.append("  - Face [").append(String(face.id)).append("] has a loop with fewer than 3 edges.\n");
            return true;
        }

        const dominantCoordinate = PolyhedralBoundedSolidGeometricValidator.dominantCoordinateForFace(face);
        let i: number;
        let j: number;
        for (i = 0; i < n; i++) {
            const heA = loop.halfEdgesList.get(i)!;
            const heANext = heA.next();
            if (heANext === null) {
                msg.append("  - Face [")
                    .append(String(face.id))
                    .append("] has a non-closed loop during strict validation.\n");
                return true;
            }
            const a1 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                heA.startingVertex.position,
                dominantCoordinate,
            );
            const a2 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                heANext.startingVertex.position,
                dominantCoordinate,
            );

            for (j = i + 1; j < n; j++) {
                if (j === (i + 1) % n || i === (j + 1) % n) {
                    continue;
                }
                const heB = loop.halfEdgesList.get(j)!;
                const heBNext = heB.next();
                if (heBNext === null) {
                    msg.append("  - Face [")
                        .append(String(face.id))
                        .append("] has a non-closed loop during strict validation.\n");
                    return true;
                }
                const b1 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                    heB.startingVertex.position,
                    dominantCoordinate,
                );
                const b2 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                    heBNext.startingVertex.position,
                    dominantCoordinate,
                );

                if (PolyhedralBoundedSolidGeometricValidator.segmentsIntersect2D(a1, a2, b1, b2, numericContext)) {
                    msg.append("  - Face [").append(String(face.id)).append("] has a self-intersecting loop.\n");
                    return true;
                }
            }
        }
        return false;
    }

    private static loopsIntersect(
        face: _PolyhedralBoundedSolidFace,
        loopA: _PolyhedralBoundedSolidLoop,
        loopB: _PolyhedralBoundedSolidLoop,
        numericContext: ToleranceContext,
        msg: StringBuilder,
    ): boolean {
        const dominantCoordinate = PolyhedralBoundedSolidGeometricValidator.dominantCoordinateForFace(face);
        let i: number;
        let j: number;
        for (i = 0; i < loopA.halfEdgesList.size(); i++) {
            const heA = loopA.halfEdgesList.get(i)!;
            const heANext = heA.next();
            if (heANext === null) {
                msg.append("  - Face [")
                    .append(String(face.id))
                    .append("] has a non-closed loop during strict validation.\n");
                return true;
            }
            const a1 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                heA.startingVertex.position,
                dominantCoordinate,
            );
            const a2 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                heANext.startingVertex.position,
                dominantCoordinate,
            );

            for (j = 0; j < loopB.halfEdgesList.size(); j++) {
                const heB = loopB.halfEdgesList.get(j)!;
                const heBNext = heB.next();
                if (heBNext === null) {
                    msg.append("  - Face [")
                        .append(String(face.id))
                        .append("] has a non-closed loop during strict validation.\n");
                    return true;
                }
                const b1 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                    heB.startingVertex.position,
                    dominantCoordinate,
                );
                const b2 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                    heBNext.startingVertex.position,
                    dominantCoordinate,
                );
                if (PolyhedralBoundedSolidGeometricValidator.segmentsIntersect2D(a1, a2, b1, b2, numericContext)) {
                    msg.append("  - Face [").append(String(face.id)).append("] has intersecting loops.\n");
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
    public static validateLoopsStrict(solid: PolyhedralBoundedSolid, msg: StringBuilder): boolean;
    /**
    Enforces strict geometric loop consistency for the planar polygons assumed
    by [MANT1988].10.2.1 and manipulated by the geometric tools of chapter
    [MANT1988].13.
    */
    public static validateLoopsStrict(
        solid: PolyhedralBoundedSolid,
        numericContext: ToleranceContext,
        msg: StringBuilder,
    ): boolean;
    public static validateLoopsStrict(
        solid: PolyhedralBoundedSolid,
        numericContextOrMsg: ToleranceContext | StringBuilder,
        maybeMsg?: StringBuilder,
    ): boolean {
        if (maybeMsg === undefined) {
            return PolyhedralBoundedSolidGeometricValidator.validateLoopsStrict(
                solid,
                PolyhedralBoundedSolidNumericPolicy.forSolid(solid),
                numericContextOrMsg as StringBuilder,
            );
        }
        const numericContext = numericContextOrMsg as ToleranceContext;
        const msg = maybeMsg;
        let i: number;
        let j: number;
        let k: number;
        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const face = solid.getPolygonsList().get(i)!;
            if (face.getContainingPlane() === null) {
                msg.append("  - Face [")
                    .append(String(face.id))
                    .append("] has no containing plane for strict checks.\n");
                return false;
            }

            for (j = 0; j < face.boundariesList.size(); j++) {
                const loop = face.boundariesList.get(j)!;
                if (PolyhedralBoundedSolidGeometricValidator.loopHasSelfIntersection(face, loop, numericContext, msg)) {
                    return false;
                }
            }
            for (j = 0; j < face.boundariesList.size(); j++) {
                for (k = j + 1; k < face.boundariesList.size(); k++) {
                    if (
                        PolyhedralBoundedSolidGeometricValidator.loopsIntersect(
                            face,
                            face.boundariesList.get(j)!,
                            face.boundariesList.get(k)!,
                            numericContext,
                            msg,
                        )
                    ) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    private static facesAreCoplanar(
        faceA: _PolyhedralBoundedSolidFace,
        _faceB: _PolyhedralBoundedSolidFace,
        planeA: InfinitePlane | null,
        planeB: InfinitePlane | null,
        numericContext: ToleranceContext,
    ): boolean {
        if (planeA === null || planeB === null) {
            return false;
        }

        let nA = planeA.getNormal().multiply(1.0);
        let nB = planeB.getNormal().multiply(1.0);
        nA = nA.normalized();
        nB = nB.normalized();
        if (Math.abs(Math.abs(nA.dotProduct(nB)) - 1.0) > numericContext.coplanarDotTolerance()) {
            return false;
        }

        for (let i = 0; i < faceA.boundariesList.size(); i++) {
            const loop = faceA.boundariesList.get(i)!;
            if (loop.halfEdgesList.size() > 0) {
                const p = loop.halfEdgesList.get(0)!.startingVertex.position;
                return Math.abs(planeB.pointDistance(p)) <= numericContext.bigEpsilon();
            }
        }
        return false;
    }

    private static segmentSharesEndpoint(
        a: _PolyhedralBoundedSolidHalfEdge,
        b: _PolyhedralBoundedSolidHalfEdge,
    ): boolean {
        const an = a.next();
        const bn = b.next();
        if (an === null || bn === null) {
            return false;
        }
        const a0: _PolyhedralBoundedSolidVertex = a.startingVertex;
        const a1: _PolyhedralBoundedSolidVertex = an.startingVertex;
        const b0: _PolyhedralBoundedSolidVertex = b.startingVertex;
        const b1: _PolyhedralBoundedSolidVertex = bn.startingVertex;
        return a0 === b0 || a0 === b1 || a1 === b0 || a1 === b1;
    }

    private static vertexStrictlyInsideFace(
        v: _PolyhedralBoundedSolidVertex,
        face: _PolyhedralBoundedSolidFace,
        faceGeometry: PolyhedralBoundedSolidFaceGeometry,
        numericContext: ToleranceContext,
    ): boolean {
        const plane = faceGeometry.getPlane()!;
        if (plane.doContainmentTest(v.position, numericContext.bigEpsilon()) !== Geometry.LIMIT) {
            return false;
        }
        if (faceGeometry.projectionExcludes(v.position, numericContext.bigEpsilon())) {
            return false;
        }
        return face.testPointInside(v.position, numericContext.bigEpsilon(), plane) === Geometry.INSIDE;
    }

    private static edgePiercesFaceInterior(
        he: _PolyhedralBoundedSolidHalfEdge,
        face: _PolyhedralBoundedSolidFace,
        faceGeometry: PolyhedralBoundedSolidFaceGeometry,
        numericContext: ToleranceContext,
    ): boolean {
        const next = he.next();
        const plane = faceGeometry.getPlane();
        if (next === null || plane === null) {
            return false;
        }

        const p0 = he.startingVertex.position;
        const p1 = next.startingVertex.position;
        const d0 = plane.pointDistance(p0);
        const d1 = plane.pointDistance(p1);

        if (Math.abs(d0) <= numericContext.bigEpsilon() && Math.abs(d1) <= numericContext.bigEpsilon()) {
            return false;
        }
        if (d0 * d1 > 0) {
            return false;
        }

        const denom = d0 - d1;
        if (PolyhedralBoundedSolidNumericPolicy.isZero(denom, numericContext)) {
            return false;
        }
        const t = d0 / denom;
        if (!PolyhedralBoundedSolidNumericPolicy.unitIntervalContainsStrictly(t, numericContext)) {
            return false;
        }

        const p = p0.add(p1.subtract(p0).multiply(t));
        if (faceGeometry.projectionExcludes(p, numericContext.bigEpsilon())) {
            return false;
        }
        return face.testPointInside(p, numericContext.bigEpsilon(), plane) === Geometry.INSIDE;
    }

    private static facesHaveImproperIntersection(
        faceA: _PolyhedralBoundedSolidFace,
        faceB: _PolyhedralBoundedSolidFace,
        geometryA: PolyhedralBoundedSolidFaceGeometry,
        geometryB: PolyhedralBoundedSolidFaceGeometry,
        numericContext: ToleranceContext,
        msg: StringBuilder,
    ): boolean {
        let i: number;
        let j: number;
        let k: number;
        let he: _PolyhedralBoundedSolidHalfEdge;

        for (i = 0; i < faceA.boundariesList.size(); i++) {
            const loop = faceA.boundariesList.get(i)!;
            for (j = 0; j < loop.halfEdgesList.size(); j++) {
                he = loop.halfEdgesList.get(j)!;
                if (
                    geometryB.getPlane() !== null &&
                    PolyhedralBoundedSolidGeometricValidator.vertexStrictlyInsideFace(
                        he.startingVertex,
                        faceB,
                        geometryB,
                        numericContext,
                    )
                ) {
                    msg.append("  - Faces [")
                        .append(String(faceA.id))
                        .append("] and [")
                        .append(String(faceB.id))
                        .append("] intersect improperly.\n");
                    return true;
                }
                if (
                    PolyhedralBoundedSolidGeometricValidator.edgePiercesFaceInterior(
                        he,
                        faceB,
                        geometryB,
                        numericContext,
                    )
                ) {
                    msg.append("  - Faces [")
                        .append(String(faceA.id))
                        .append("] and [")
                        .append(String(faceB.id))
                        .append("] intersect improperly.\n");
                    return true;
                }
            }
        }

        for (i = 0; i < faceB.boundariesList.size(); i++) {
            const loop = faceB.boundariesList.get(i)!;
            for (j = 0; j < loop.halfEdgesList.size(); j++) {
                he = loop.halfEdgesList.get(j)!;
                if (
                    geometryA.getPlane() !== null &&
                    PolyhedralBoundedSolidGeometricValidator.vertexStrictlyInsideFace(
                        he.startingVertex,
                        faceA,
                        geometryA,
                        numericContext,
                    )
                ) {
                    msg.append("  - Faces [")
                        .append(String(faceA.id))
                        .append("] and [")
                        .append(String(faceB.id))
                        .append("] intersect improperly.\n");
                    return true;
                }
                if (
                    PolyhedralBoundedSolidGeometricValidator.edgePiercesFaceInterior(
                        he,
                        faceA,
                        geometryA,
                        numericContext,
                    )
                ) {
                    msg.append("  - Faces [")
                        .append(String(faceA.id))
                        .append("] and [")
                        .append(String(faceB.id))
                        .append("] intersect improperly.\n");
                    return true;
                }
            }
        }

        if (
            PolyhedralBoundedSolidGeometricValidator.facesAreCoplanar(
                faceA,
                faceB,
                geometryA.getPlane(),
                geometryB.getPlane(),
                numericContext,
            )
        ) {
            const dominantCoordinate = PolyhedralBoundedSolidGeometricValidator.dominantCoordinateForFace(faceA);
            for (i = 0; i < faceA.boundariesList.size(); i++) {
                const loopA = faceA.boundariesList.get(i)!;
                for (j = 0; j < loopA.halfEdgesList.size(); j++) {
                    const heA = loopA.halfEdgesList.get(j)!;
                    const heANext = heA.next();
                    if (heANext === null) {
                        continue;
                    }
                    const a1 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                        heA.startingVertex.position,
                        dominantCoordinate,
                    );
                    const a2 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                        heANext.startingVertex.position,
                        dominantCoordinate,
                    );

                    for (k = 0; k < faceB.boundariesList.size(); k++) {
                        const loopB = faceB.boundariesList.get(k)!;
                        for (let m = 0; m < loopB.halfEdgesList.size(); m++) {
                            const heB = loopB.halfEdgesList.get(m)!;
                            const heBNext = heB.next();
                            if (heBNext === null) {
                                continue;
                            }
                            if (heA.parentEdge === heB.parentEdge) {
                                continue;
                            }
                            const b1 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                                heB.startingVertex.position,
                                dominantCoordinate,
                            );
                            const b2 = PolyhedralBoundedSolidGeometricValidator.projectPointTo2D(
                                heBNext.startingVertex.position,
                                dominantCoordinate,
                            );
                            if (
                                PolyhedralBoundedSolidGeometricValidator.segmentsIntersect2D(
                                    a1,
                                    a2,
                                    b1,
                                    b2,
                                    numericContext,
                                ) &&
                                !PolyhedralBoundedSolidGeometricValidator.segmentSharesEndpoint(heA, heB)
                            ) {
                                msg.append("  - Faces [")
                                    .append(String(faceA.id))
                                    .append("] and [")
                                    .append(String(faceB.id))
                                    .append("] intersect improperly.\n");
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
    public static validateFaceIntersectionsStrict(solid: PolyhedralBoundedSolid, msg: StringBuilder): boolean;
    /**
    Applies strict face/face intersection checks to preserve the validity
    condition from [MANT1988].15.2, criterion 3, using predicates from chapter
    [MANT1988].13.
    */
    public static validateFaceIntersectionsStrict(
        solid: PolyhedralBoundedSolid,
        numericContext: ToleranceContext,
        msg: StringBuilder,
    ): boolean;
    public static validateFaceIntersectionsStrict(
        solid: PolyhedralBoundedSolid,
        numericContextOrMsg: ToleranceContext | StringBuilder,
        maybeMsg?: StringBuilder,
    ): boolean {
        if (maybeMsg === undefined) {
            return PolyhedralBoundedSolidGeometricValidator.validateFaceIntersectionsStrict(
                solid,
                PolyhedralBoundedSolidNumericPolicy.forSolid(solid),
                numericContextOrMsg as StringBuilder,
            );
        }
        const numericContext = numericContextOrMsg as ToleranceContext;
        const msg = maybeMsg;
        let i: number;
        let j: number;
        const n = solid.getPolygonsList().size();
        const faces: _PolyhedralBoundedSolidFace[] = new Array<_PolyhedralBoundedSolidFace>(n);
        let geometry: PolyhedralBoundedSolidFaceGeometry[];
        const margin: number[] = new Array<number>(n).fill(0.0);
        let coplanarSlack: number;

        // Stage G (Fable5.2Analysis.md): planes are computed once per pass
        // instead of once per face pair and per point, and face pairs whose
        // grown boxes are apart are skipped. Every improper intersection has
        // a witness point that lies in both grown boxes: a vertex of one face
        // inside the other, the point where an edge pierces a face, or a
        // crossing of two coplanar edges. For the last one the faces may be
        // tilted by up to acos(1 - coplanarDotTolerance), which coplanarSlack
        // bounds over the whole solid.
        geometry = PolyhedralBoundedSolidFaceGeometry.of(solid);
        coplanarSlack =
            2.0 *
            Math.sqrt(2.0 * numericContext.coplanarDotTolerance()) *
            PolyhedralBoundedSolidGeometricValidator.solidDiagonal(solid);
        for (i = 0; i < n; i++) {
            faces[i] = solid.getPolygonsList().get(i)!;
            margin[i] = geometry[i]!.containmentMargin(numericContext.bigEpsilon());
        }
        for (i = 0; i < n; i++) {
            for (j = i + 1; j < n; j++) {
                if (geometry[i]!.boxSeparatedFrom(geometry[j]!, margin[i]! + margin[j]! + coplanarSlack)) {
                    continue;
                }
                if (
                    PolyhedralBoundedSolidGeometricValidator.facesHaveImproperIntersection(
                        faces[i]!,
                        faces[j]!,
                        geometry[i]!,
                        geometry[j]!,
                        numericContext,
                        msg,
                    )
                ) {
                    return false;
                }
            }
        }
        return true;
    }

    private static solidDiagonal(solid: PolyhedralBoundedSolid): number {
        const box = solid.getMinMax();
        return Math.sqrt(
            (box[3]! - box[0]!) * (box[3]! - box[0]!) +
                (box[4]! - box[1]!) * (box[4]! - box[1]!) +
                (box[5]! - box[2]!) * (box[5]! - box[2]!),
        );
    }

    /**
    Area below which a face is collapsed: VSDK.EPSILON scaled by the square
    of the model scale, so that it equals EPSILON for models of unit size and
    is invariant under a uniform change of units.
    */
    public static collapsedFaceAreaThreshold(numericContext: ToleranceContext): number {
        return VSDK.EPSILON * numericContext.modelScale() * numericContext.modelScale();
    }

    /**
    Area of a face: half the length of the Newell vector summed over all its
    loops, so inner loops (holes, oriented the other way) are subtracted.
    */
    public static faceArea(face: _PolyhedralBoundedSolidFace): number {
        let nx = 0.0;
        let ny = 0.0;
        let nz = 0.0;
        let i: number;

        for (i = 0; i < face.boundariesList.size(); i++) {
            const loop = face.boundariesList.get(i);
            let start: _PolyhedralBoundedSolidHalfEdge;
            let he: _PolyhedralBoundedSolidHalfEdge;
            if (loop === null || loop.boundaryStartHalfEdge === null) {
                continue;
            }
            start = loop.boundaryStartHalfEdge;
            he = start;
            do {
                const next = he.next();
                if (next === null) {
                    break;
                }
                const p = he.startingVertex.position;
                const q = next.startingVertex.position;
                nx += (p.y() - q.y()) * (p.z() + q.z());
                ny += (p.z() - q.z()) * (p.x() + q.x());
                nz += (p.x() - q.x()) * (p.y() + q.y());
                he = next;
            } while (he !== start);
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
    public static validateNoCollapsedLaminae(
        solid: PolyhedralBoundedSolid,
        numericContext: ToleranceContext,
        msg: StringBuilder,
    ): boolean {
        const threshold = PolyhedralBoundedSolidGeometricValidator.collapsedFaceAreaThreshold(numericContext);
        const shellOf = new Map<_PolyhedralBoundedSolidFace, number>();
        const shells: _PolyhedralBoundedSolidFace[][] = [];
        let ok = true;
        let i: number;

        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const seed = solid.getPolygonsList().get(i)!;
            if (shellOf.has(seed)) {
                continue;
            }
            const shell: _PolyhedralBoundedSolidFace[] = [];
            const pending: _PolyhedralBoundedSolidFace[] = [];
            let pendingHead = 0;
            shellOf.set(seed, shells.length);
            pending.push(seed);
            while (pendingHead < pending.length) {
                const face = pending[pendingHead++]!;
                shell.push(face);
                for (let j = 0; j < face.boundariesList.size(); j++) {
                    const loop = face.boundariesList.get(j)!;
                    for (let k = 0; k < loop.halfEdgesList.size(); k++) {
                        const he = loop.halfEdgesList.get(k)!;
                        const mate = he.parentEdge === null ? null : he.mirrorHalfEdge();
                        if (mate === null || mate.parentLoop === null) {
                            continue;
                        }
                        const other = mate.parentLoop.parentFace;
                        if (other !== null && !shellOf.has(other)) {
                            shellOf.set(other, shells.length);
                            pending.push(other);
                        }
                    }
                }
            }
            shells.push(shell);
        }

        for (i = 0; i < shells.length; i++) {
            const shell = shells[i]!;
            if (shell.length !== 2) {
                continue;
            }
            const area = PolyhedralBoundedSolidGeometricValidator.faceArea(shell[0]!);
            if (area < threshold) {
                msg.append("  - Lamina of faces [")
                    .append(String(shell[0]!.id))
                    .append("] and [")
                    .append(String(shell[1]!.id))
                    .append("] is collapsed: area ")
                    .append(Double.toString(area))
                    .append(" < ")
                    .append(Double.toString(threshold))
                    .append(".\n");
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
    public static validateNoCoincidentVertices(
        solid: PolyhedralBoundedSolid,
        context: ToleranceContext,
        msg: StringBuilder,
    ): boolean {
        let i: number;
        let j: number;
        let ok: boolean;
        let vi: _PolyhedralBoundedSolidVertex | null;
        let vj: _PolyhedralBoundedSolidVertex | null;

        ok = true;
        for (i = 0; i < solid.getVerticesList().size(); i++) {
            vi = solid.getVerticesList().get(i);
            if (vi === null || vi.position === null) {
                continue;
            }
            for (j = i + 1; j < solid.getVerticesList().size(); j++) {
                vj = solid.getVerticesList().get(j);
                if (vj === null || vj.position === null) {
                    continue;
                }
                if (PolyhedralBoundedSolidNumericPolicy.pointsCoincident(vi.position, vj.position, context)) {
                    msg.append("  coincident vertices: v")
                        .append(String(vi.id))
                        .append(" and v")
                        .append(String(vj.id))
                        .append(" at ")
                        .append(String(vi.position))
                        .append("\n");
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
    public static validateUniqueFaceAndVertexIds(solid: PolyhedralBoundedSolid, msg: StringBuilder): boolean {
        let ok: boolean;
        let i: number;
        let j: number;
        let fi: _PolyhedralBoundedSolidFace | null;
        let fj: _PolyhedralBoundedSolidFace | null;
        let vi: _PolyhedralBoundedSolidVertex | null;
        let vj: _PolyhedralBoundedSolidVertex | null;

        ok = true;

        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            fi = solid.getPolygonsList().get(i);
            if (fi === null) {
                continue;
            }
            if (fi.id > solid.getMaxFaceId()) {
                msg.append("  face id ")
                    .append(String(fi.id))
                    .append(" exceeds maxFaceId=")
                    .append(String(solid.getMaxFaceId()))
                    .append("\n");
                ok = false;
            }
            for (j = i + 1; j < solid.getPolygonsList().size(); j++) {
                fj = solid.getPolygonsList().get(j);
                if (fj !== null && fi.id === fj.id) {
                    msg.append("  duplicate face id=").append(String(fi.id)).append("\n");
                    ok = false;
                }
            }
        }

        for (i = 0; i < solid.getVerticesList().size(); i++) {
            vi = solid.getVerticesList().get(i);
            if (vi === null) {
                continue;
            }
            if (vi.id > solid.getMaxVertexId()) {
                msg.append("  vertex id ")
                    .append(String(vi.id))
                    .append(" exceeds maxVertexId=")
                    .append(String(solid.getMaxVertexId()))
                    .append("\n");
                ok = false;
            }
            for (j = i + 1; j < solid.getVerticesList().size(); j++) {
                vj = solid.getVerticesList().get(j);
                if (vj !== null && vi.id === vj.id) {
                    msg.append("  duplicate vertex id=").append(String(vi.id)).append("\n");
                    ok = false;
                }
            }
        }

        return ok;
    }
}
