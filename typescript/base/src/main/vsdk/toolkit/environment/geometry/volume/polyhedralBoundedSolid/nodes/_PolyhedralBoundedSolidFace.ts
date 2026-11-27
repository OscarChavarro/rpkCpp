//= References:                                                             =
//= [GLAS1989] Glassner, Andrew. "An introduction to ray tracing",          =
//=     Academic Press, 1989.                                               =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { CircularArrayList } from "../../../../../common/dataStructures/CircularArrayList.js";
import { FundamentalEntity } from "../../../../../common/FundamentalEntity.js";
import type { ArrayLinkable } from "../../../../../common/dataStructures/ArrayLinkable.js";
import { Vector3Dd } from "../../../../../common/linealAlgebra/Vector3Dd.js";
import { ArrayListOfDoubles } from "../../../../../common/dataStructures/ArrayListOfDoubles.js";
import { Geometry } from "../../../Geometry.js";
import type { PolyhedralBoundedSolid } from "../PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidNumericPolicy } from "../PolyhedralBoundedSolidNumericPolicy.js";
import { InfinitePlane } from "../../../surface/InfinitePlane.js";
import { ComputationalGeometry } from "../../../../../processing/ComputationalGeometry.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./_PolyhedralBoundedSolidHalfEdge.js";
import type { _PolyhedralBoundedSolidLoop } from "./_PolyhedralBoundedSolidLoop.js";
import type { _PolyhedralBoundedSolidVertex } from "./_PolyhedralBoundedSolidVertex.js";

/**
As noted in [MANT1988].10.2.1, class `_PolyhedralBoundedSolidFace` represents
one planar face of the polyhedron represented by the half-edge data
structure in a `PolyhedralBoundedSolid`. A face is defined as a planar
polygon whose interior is connected, considering that could be convex or
concave, with or without holes (but without "islands", in which case there
are more than one polygon), and based in this, a polygon can have more than
one polygonal boundary.

Note that in current implementation, the first loop in the list of boundaries
is the outer boundary, and the others are "rings" or hole loops.

Note that in the sake of simplify and eficiency current programming
implementation of this class exhibit public access attributes. It is important
to note that those attributes will only be accessed directly from related
classes in the same package
(vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes) and
from methods in the `PolyhedralBoundedSolid` class, and that they should
not be used from outer classes.
*/
export class _PolyhedralBoundedSolidFace extends FundamentalEntity implements ArrayLinkable {
    /// Place of this element in the list that holds it, kept up to date by
    /// the list (see `ArrayLinkable`).
    private listOwner: object | null = null;
    private listIndex = -1;

    public getListOwner(): object | null {
        return this.listOwner;
    }

    public getListIndex(): number {
        return this.listIndex;
    }

    public setListSlot(owner: object | null, index: number): void {
        this.listOwner = owner;
        this.listIndex = index;
    }

    /// Defined as presented in [MANT1988].10.2.1
    public id!: number;

    /// Defined as presented in [MANT1988].10.2.1
    public parentSolid!: PolyhedralBoundedSolid;

    /// Each face should have at least one loop, corresponding to the
    /// external boundary. Each subsequent loop will be interpreted as a ring.
    /// Defined as presented in [MANT1988].10.2.1
    public boundariesList!: CircularArrayList<_PolyhedralBoundedSolidLoop>;

    //=================================================================

    public constructor(parent: PolyhedralBoundedSolid, id: number) {
        super();
        this.init(parent, id);
    }

    private init(parent: PolyhedralBoundedSolid, id: number): void {
        this.id = id;
        this.parentSolid = parent;
        this.parentSolid.getPolygonsList().add(this);
        this.boundariesList = new CircularArrayList<_PolyhedralBoundedSolidLoop>();
    }

    private static boundaryLoopAreaMagnitude(loop: _PolyhedralBoundedSolidLoop | null): number {
        let start: _PolyhedralBoundedSolidHalfEdge;
        let he: _PolyhedralBoundedSolidHalfEdge;
        let normalAccumulator: Vector3Dd;

        if (loop === null || loop.boundaryStartHalfEdge === null) {
            return 0.0;
        }

        start = loop.boundaryStartHalfEdge;
        he = start;
        normalAccumulator = new Vector3Dd();
        do {
            const p = he.startingVertex.position;
            const q = he.next()!.startingVertex.position;

            normalAccumulator = normalAccumulator.add(
                new Vector3Dd(
                    (p.y() - q.y()) * (p.z() + q.z()),
                    (p.z() - q.z()) * (p.x() + q.x()),
                    (p.x() - q.x()) * (p.y() + q.y()),
                ),
            );
            he = he.next()!;
        } while (he !== start);

        return normalAccumulator.length();
    }

    private selectLoopForPlaneCalculation(): _PolyhedralBoundedSolidLoop | null {
        let selectedLoop: _PolyhedralBoundedSolidLoop | null;
        let i: number;
        let maxAreaMagnitude: number;

        if (this.boundariesList.size() < 1) {
            return null;
        }

        selectedLoop = this.boundariesList.get(0);
        maxAreaMagnitude = _PolyhedralBoundedSolidFace.boundaryLoopAreaMagnitude(selectedLoop);

        for (i = 1; i < this.boundariesList.size(); i++) {
            const candidate = this.boundariesList.get(i);
            const candidateAreaMagnitude = _PolyhedralBoundedSolidFace.boundaryLoopAreaMagnitude(candidate);
            if (candidateAreaMagnitude > maxAreaMagnitude) {
                maxAreaMagnitude = candidateAreaMagnitude;
                selectedLoop = candidate;
            }
        }

        return selectedLoop;
    }

    /**
    Find the halfedge from vertex `vn1` to vertex `vn2`.
    Returns null if halfedge not found, or current founded halfedge otherwise.
    Build based over function `fhe` in program [MANT1988].11.9.

    Java overload `findHalfEdge(int vn1)`: find the first halfedge originating
    from vertex `vn1`. Returns null if halfedge not found, or current founded
    halfedge otherwise.
    @param vn1
    @param vn2
    @return requested half edge
    */
    public findHalfEdge(vn1: number, vn2?: number): _PolyhedralBoundedSolidHalfEdge | null {
        let loop: _PolyhedralBoundedSolidLoop;
        let he: _PolyhedralBoundedSolidHalfEdge | null;
        let i: number;

        if (vn2 !== undefined) {
            for (i = 0; i < this.boundariesList.size(); i++) {
                loop = this.boundariesList.get(i)!;
                he = loop.halfEdgeVertices(vn1, vn2);
                if (he !== null) {
                    return he;
                }
            }
            return null;
        }

        for (i = 0; i < this.boundariesList.size(); i++) {
            loop = this.boundariesList.get(i)!;
            he = loop.firstHalfEdgeAtVertex(vn1);
            if (he !== null) {
                return he;
            }
        }
        return null;
    }

    /**
    Compatibility hook for callers that used to refresh a cached containing
    plane. Faces no longer store that plane; use getContainingPlane() to
    compute it for the current topology.
    @return true when a containing plane cannot be calculated.
    */
    public calculatePlane(): boolean {
        return this.getContainingPlane() === null;
    }

    public getContainingPlane(): InfinitePlane | null {
        const numericContext = PolyhedralBoundedSolidNumericPolicy.forFace(this);
        let plane = this.calculatePlaneByNewell(numericContext.bigEpsilon());
        if (plane === null) {
            plane = this.calculatePlaneByCorner(numericContext.bigEpsilon());
        }
        return plane;
    }

    /**
    Computes the face plane using the Newell method: accumulates the sum of
    (p_i - p_{i+1}) x (p_i + p_{i+1}) / 2 over all loop vertices to obtain a
    normal proportional to the signed area, then uses the vertex centroid as the
    reference point.  More robust than picking three vertices for non-convex or
    nearly-degenerate faces produced by boolean operations.
    @param tolerance degenerate-face threshold; returns null when the computed
    normal length is at or below this value.
    @return a plane containing the face, or null when the face is degenerate.
    */
    private calculatePlaneByNewell(tolerance: number): InfinitePlane | null {
        let he: _PolyhedralBoundedSolidHalfEdge;
        let p: Vector3Dd;
        let q: Vector3Dd;

        const loop = this.selectLoopForPlaneCalculation();
        if (loop === null || loop.boundaryStartHalfEdge === null) {
            return null;
        }

        he = loop.boundaryStartHalfEdge;
        const start = he;
        let nx = 0.0;
        let ny = 0.0;
        let nz = 0.0;
        let cx = 0.0;
        let cy = 0.0;
        let cz = 0.0;
        let count = 0;

        do {
            p = he.startingVertex.position;
            q = he.next()!.startingVertex.position;
            nx += (p.y() - q.y()) * (p.z() + q.z());
            ny += (p.z() - q.z()) * (p.x() + q.x());
            nz += (p.x() - q.x()) * (p.y() + q.y());
            cx += p.x();
            cy += p.y();
            cz += p.z();
            count++;
            he = he.next()!;
        } while (he !== start);

        if (count < 3) {
            return null;
        }

        let normal = new Vector3Dd(nx, ny, nz);
        const length = normal.length();
        if (length <= tolerance) {
            return null;
        }
        // The Newell normal measures twice the area. Vector3Dd.normalized()
        // leaves vectors shorter than VSDK.EPSILON unchanged, so a face of a
        // model at 10^-3 got a plane with a normal of length 10^-6 and every
        // distance to it shrank a millionfold. Normalize explicitly.
        normal = normal.multiply(1.0 / length);

        const centroid = new Vector3Dd(cx / count, cy / count, cz / count);
        return new InfinitePlane(normal, centroid);
    }

    /**
    @deprecated Kept for git-history reference; replaced by calculatePlaneByNewell.
    Current implementation takes in to account only the first loop.
    @return a plane containing the face, or null when it cannot be calculated.
    */
    private calculatePlaneByCorner(tolerance: number): InfinitePlane | null {
        const numericContext = PolyhedralBoundedSolidNumericPolicy.forFace(this);
        const nonColinearDotTolerance = numericContext.coplanarDotTolerance();
        let he: _PolyhedralBoundedSolidHalfEdge | null;
        let p0 = new Vector3Dd();
        let p1: Vector3Dd;
        let a = new Vector3Dd();
        let b = new Vector3Dd();
        let n1: Vector3Dd;
        let temp: Vector3Dd;
        let readyVecA: boolean;
        let readyVecB: boolean;
        let dotP: number;
        //domPlane: 1=xy, 2=xz, 3=yz
        let domPlane: number;
        let vPrev: Vector3Dd;
        let vNext = new Vector3Dd();

        if (this.boundariesList.size() < 1) {
            return null;
        }
        const loop = this.selectLoopForPlaneCalculation();
        if (loop === null) {
            return null;
        }
        he = loop.boundaryStartHalfEdge;
        if (he === null) {
            // Loop without starting halfedge
            return null;
        }
        const heStart = he;

        // Calculate temporal normal (the sense may not be the correct), to find
        // the dominant plane.
        // The superior point is calculated too.
        readyVecA = false;
        readyVecB = false;

        do {
            //Obtain any two non collinear vectors
            p0 = he!.startingVertex.position;
            p1 = he!.next()!.startingVertex.position;
            temp = p1.subtract(p0);
            if (!readyVecA) {
                if (temp.length() > tolerance) {
                    a = new Vector3Dd(temp);
                    a = a.normalized();
                    readyVecA = true;
                }
            } else if (!readyVecB) {
                if (temp.length() > tolerance) {
                    temp = temp.normalized();
                    dotP = Math.abs(temp.dotProduct(a));
                    if (dotP < 1 - nonColinearDotTolerance) {
                        b = new Vector3Dd(temp);
                        readyVecB = true;
                    }
                }
            }
            he = he!.next();
        } while (he !== heStart && !readyVecB);
        if (a.length() === 0 || b.length() === 0) {
            // Any vector is zero.
            return null;
        }
        n1 = a.crossProduct(b); //Temporal normal.
        // Special case: triangle
        if (loop.halfEdgesList.size() === 3) {
            n1 = n1.normalized();
            return new InfinitePlane(n1, p0);
        }
        //Test for dominant plane.
        //domPlane: 1=xy, 2=xz, 3=yz
        if (Math.abs(n1.z()) > Math.abs(n1.x())) {
            if (Math.abs(n1.z()) > Math.abs(n1.y())) {
                domPlane = 1;
            } else {
                domPlane = 2;
            }
        } else if (Math.abs(n1.x()) > Math.abs(n1.y())) {
            domPlane = 3;
        } else {
            domPlane = 2;
        }
        //Find inferior point of face, given the dominant plane.
        he = loop.boundaryStartHalfEdge;
        let heInferior = he!;
        he = he!.next();
        while (he !== heStart) {
            p0 = he!.startingVertex.position;
            switch (domPlane) {
                case 1: //xy plane
                    if (p0.y() < heInferior.startingVertex.position.y()) {
                        heInferior = he!;
                    }
                    break;
                case 2: //xz plane
                    if (p0.z() < heInferior.startingVertex.position.z()) {
                        heInferior = he!;
                    }
                    break;
                case 3: //yz plane
                    if (p0.z() < heInferior.startingVertex.position.z()) {
                        heInferior = he!;
                    }
                    break;
            }
            he = he!.next();
        }
        // Find next and previous vectors from inferior point(previously found)
        // to calculate the plane.
        he = heInferior;
        p0 = heInferior.startingVertex.position;
        do {
            he = he!.next();
            p1 = he!.startingVertex.position;
            vNext = p1.subtract(p0);
            if (vNext.length() > tolerance) {
                vNext = vNext.normalized();
                break;
            }
        } while (he !== heInferior);
        he = heInferior;
        do {
            // The previous vector should not be collinear with the first one found.
            he = he!.previous();
            p1 = he!.startingVertex.position;
            vPrev = p1.subtract(p0);
            if (vPrev.length() > tolerance) {
                vPrev = vPrev.normalized();
                dotP = Math.abs(vPrev.dotProduct(vNext));
                if (dotP < 1 - nonColinearDotTolerance) {
                    break;
                }
            }
        } while (he !== heInferior);
        n1 = vNext.crossProduct(vPrev);
        return new InfinitePlane(n1, p0);
    }

    /**
    @coord: 1 means drop x, 2 means drop y and 3 means drop z
    */
    private dropCoordinate(input: Vector3Dd, coord: number): Vector3Dd {
        switch (coord) {
            case 1:
                // Drop X
                return new Vector3Dd(input.y(), input.z(), 0);
            case 2:
                // Drop Y
                return new Vector3Dd(input.x(), input.z(), 0);
            case 3:
            default:
                // Drop Z
                return new Vector3Dd(input.x(), input.y(), 0);
        }
    }

    /**
    Given a point p in the containing plane of this face, the method returns:
    @param p
    @param tolerance
    @return Geometry.OUTSIDE if point is outside polygon, Geometry.LIMIT if
    its in the polygon border, Geometry.INSIDE if point is inside border.
    PRE:
    - Polygon is planar
    - Point p is in the containing plane
    The structure of this algorithm follows the one outlined in
    [GLAS1989].2.3.2. with a little variation in the handlig of `sh`
    which allows this code to manage internal loops.

    Functionally, this is equivalent to procedure `contfv` proposed at
    problem [MANT1988].13.3.

    Java overload with a caller-supplied plane: variant that reuses a
    caller-supplied containing plane instead of recomputing it. Read-only
    analysis passes (point classification) test many points against the same
    unchanged face, so precomputing the plane once and threading it here avoids
    a full Newell recompute (plus tolerance-context rebuild) per point. The
    supplied plane must be `this` face's containing plane (or null to fall
    back to recomputation).
    @param plane precomputed containing plane for this face, or null.
    */
    public testPointInside(p: Vector3Dd, tolerance: number, plane?: InfinitePlane | null): number {
        if (plane === undefined) {
            return this.testPointInsideDetailed(p, tolerance).status();
        }
        return this.testPointInsideDetailed(p, tolerance, plane).status();
    }

    public testPointInsideDetailed(p: Vector3Dd, tolerance: number, plane?: InfinitePlane | null): PointInsideResult {
        if (plane === undefined) {
            return this.testPointInsideDetailed(p, tolerance, this.getContainingPlane());
        }

        let nc: number; // Number of crossings
        let sh: number; // Sign holder for vertex crossings
        let nsh: number; // Next sign holder for vertex crossings

        //-----------------------------------------------------------------
        //- 1. For all vertices in face, project them in to dominant
        //- coordinate's plane
        const polygon2Du = new ArrayListOfDoubles(100);
        const polygon2Dv = new ArrayListOfDoubles(100);
        const polygon2Dh: _PolyhedralBoundedSolidHalfEdge[] = [];
        const polygon2Dvv: _PolyhedralBoundedSolidVertex[] = [];
        let u: number;
        let v: number;
        let projectedPoint: Vector3Dd;
        let dominantCoordinate: number;
        let i: number;

        if (plane === null) {
            plane = this.getContainingPlane();
        }
        const n = plane!.getNormal();

        if (Math.abs(n.x()) >= Math.abs(n.y()) && Math.abs(n.x()) >= Math.abs(n.z())) {
            dominantCoordinate = 1;
        } else if (Math.abs(n.y()) >= Math.abs(n.x()) && Math.abs(n.y()) >= Math.abs(n.z())) {
            dominantCoordinate = 2;
        } else {
            dominantCoordinate = 3;
        }

        let he: _PolyhedralBoundedSolidHalfEdge | null;

        for (i = 0; i < this.boundariesList.size(); i++) {
            let heOld: _PolyhedralBoundedSolidHalfEdge;

            const loop = this.boundariesList.get(i)!;
            he = loop.boundaryStartHalfEdge;
            if (he === null) {
                // Loop without starting halfedge
                return new PointInsideResult(Geometry.OUTSIDE, null, null);
            }
            const heStart = he;
            do {
                if (Vector3Dd.distance(p, he.startingVertex.position) < 2 * tolerance) {
                    return new PointInsideResult(Geometry.LIMIT, null, he.startingVertex);
                }

                projectedPoint = this.dropCoordinate(he.startingVertex.position, dominantCoordinate);
                polygon2Du.add(projectedPoint.x());
                polygon2Dv.add(projectedPoint.y());
                polygon2Dh.push(he);
                heOld = he;
                polygon2Dvv.push(he.startingVertex);
                he = he.next();
                if (he === null) {
                    // Loop is not closed!
                    return new PointInsideResult(Geometry.OUTSIDE, null, null);
                }
                projectedPoint = this.dropCoordinate(he.startingVertex.position, dominantCoordinate);
                polygon2Du.add(projectedPoint.x());
                polygon2Dv.add(projectedPoint.y());
                polygon2Dvv.push(he.startingVertex);

                if (Vector3Dd.distance(p, he.startingVertex.position) < 2 * tolerance) {
                    return new PointInsideResult(Geometry.LIMIT, null, he.startingVertex);
                }

                if (
                    ComputationalGeometry.lineSegmentContainmentTest(
                        heOld.startingVertex.position,
                        he.startingVertex.position,
                        p,
                        tolerance,
                    ) === Geometry.LIMIT
                ) {
                    return new PointInsideResult(Geometry.LIMIT, heOld, null);
                }
            } while (he !== heStart);
        }

        projectedPoint = this.dropCoordinate(p, dominantCoordinate);
        u = projectedPoint.x();
        v = projectedPoint.y();

        //-----------------------------------------------------------------
        //- 2. Translate the 2D polygon such that the intersection point is
        //- in the origin
        for (i = 0; i < polygon2Du.size(); i++) {
            let val: number;
            val = polygon2Du.get(i) - u;
            polygon2Du.set(i, val);
            val = polygon2Dv.get(i) - v;
            polygon2Dv.set(i, val);
        }
        nc = 0;

        //-----------------------------------------------------------------
        //- 3. Iterate edges
        let ua: number;
        let va: number;
        let ub: number;
        let vb: number;

        for (i = 0; i < polygon2Du.size() - 1; i += 2) {
            // This iteration tests the line segment (ua, va) - (ub, vb)
            ua = polygon2Du.get(i);
            va = polygon2Dv.get(i);
            ub = polygon2Du.get(i + 1);
            vb = polygon2Dv.get(i + 1);

            // Note that testing line is (y = 0), so "segment crossed" can be
            // detected as a sign change in the v dimension.

            // First, calculate the va and vb signs in sh and nsh respectively
            if (va < 0) {
                sh = -1;
            } else {
                sh = 1;
            }
            if (vb < 0) {
                nsh = -1;
            } else {
                nsh = 1;
            }

            // If a sign change in the v dimension occurs, then report cross...
            if (sh !== nsh) {
                // But taking into account the special case crossing occurring
                // over a vertex
                if (ua >= 0 && ub >= 0) {
                    nc++;
                } else if (ua >= 0 || ub >= 0) {
                    if (ua - (va * (ub - ua)) / (vb - va) > 0) {
                        nc++;
                    }
                }
            }
        }

        if (nc % 2 === 1) {
            return new PointInsideResult(Geometry.INSIDE, null, null);
        }

        return new PointInsideResult(Geometry.OUTSIDE, null, null);
    }

    public revert(): void {
        let i: number;

        for (i = 0; i < this.boundariesList.size(); i++) {
            this.boundariesList.get(i)!.revert();
        }
    }

    public override toString(): string {
        let msg: string;

        msg = "Face id [" + this.id + "], " + this.boundariesList.size() + " loops.";

        return msg;
    }
}

export namespace _PolyhedralBoundedSolidFace {
    export class PointInsideResult {
        private readonly statusValue: number;
        private readonly intersectedHalfedgeValue: _PolyhedralBoundedSolidHalfEdge | null;
        private readonly intersectedVertexValue: _PolyhedralBoundedSolidVertex | null;

        public constructor(
            status: number,
            intersectedHalfedge: _PolyhedralBoundedSolidHalfEdge | null,
            intersectedVertex: _PolyhedralBoundedSolidVertex | null,
        ) {
            this.statusValue = status;
            this.intersectedHalfedgeValue = intersectedHalfedge;
            this.intersectedVertexValue = intersectedVertex;
        }

        public status(): number {
            return this.statusValue;
        }

        public intersectedHalfedge(): _PolyhedralBoundedSolidHalfEdge | null {
            return this.intersectedHalfedgeValue;
        }

        public intersectedVertex(): _PolyhedralBoundedSolidVertex | null {
            return this.intersectedVertexValue;
        }
    }
}

export const PointInsideResult = _PolyhedralBoundedSolidFace.PointInsideResult;
export type PointInsideResult = _PolyhedralBoundedSolidFace.PointInsideResult;
