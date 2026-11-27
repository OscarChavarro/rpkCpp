//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { VSDK } from "../../../../common/VSDK.js";
import { Vector3Dd } from "../../../../common/linealAlgebra/Vector3Dd.js";
import type { InfinitePlane } from "../../surface/InfinitePlane.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import type { _PolyhedralBoundedSolidFace } from "./nodes/_PolyhedralBoundedSolidFace.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./nodes/_PolyhedralBoundedSolidHalfEdge.js";
import type { _PolyhedralBoundedSolidLoop } from "./nodes/_PolyhedralBoundedSolidLoop.js";

/**
Snapshot of the geometry of one face for read-only passes that query the
same unchanged faces many times (strict validation, point classification):
the containing plane, computed once, and the axis-aligned bounding box of
the face vertices (Fable5.2Analysis.md Stage G).

<p>Faces do not cache their plane, because vertex positions are public and
change outside the Euler operators (transformations, snapping). A snapshot
is only valid while its solid is not changed; build it at the start of the
pass and drop it at the end.</p>

<p>The box tests are conservative with respect to
`_PolyhedralBoundedSolidFace.testPointInside(point, tolerance, plane)`:
{@link #projectionExcludes} answers true only when that test would answer
OUTSIDE, so skipping the test then changes no result.</p>
 */
export class PolyhedralBoundedSolidFaceGeometry {
    private readonly plane: InfinitePlane | null;
    private readonly dominantCoordinate: number;
    private readonly minX: number;
    private readonly minY: number;
    private readonly minZ: number;
    private readonly maxX: number;
    private readonly maxY: number;
    private readonly maxZ: number;
    /** Largest distance from a face vertex to the plane. */
    private readonly planeDeviation: number;
    private readonly longestEdge: number;

    private constructor(face: _PolyhedralBoundedSolidFace) {
        let loop: _PolyhedralBoundedSolidLoop | null;
        let he: _PolyhedralBoundedSolidHalfEdge;
        let start: _PolyhedralBoundedSolidHalfEdge;
        let lowX = Number.POSITIVE_INFINITY;
        let lowY = Number.POSITIVE_INFINITY;
        let lowZ = Number.POSITIVE_INFINITY;
        let highX = Number.NEGATIVE_INFINITY;
        let highY = Number.NEGATIVE_INFINITY;
        let highZ = Number.NEGATIVE_INFINITY;
        let deviation = 0.0;
        let longest = 0.0;
        let i: number;

        this.plane = face.getContainingPlane();
        for (i = 0; i < face.boundariesList.size(); i++) {
            loop = face.boundariesList.get(i);
            if (loop === null || loop.boundaryStartHalfEdge === null) {
                continue;
            }
            start = loop.boundaryStartHalfEdge;
            he = start;
            do {
                const p = he.startingVertex.position;
                lowX = Math.min(lowX, p.x());
                lowY = Math.min(lowY, p.y());
                lowZ = Math.min(lowZ, p.z());
                highX = Math.max(highX, p.x());
                highY = Math.max(highY, p.y());
                highZ = Math.max(highZ, p.z());
                if (this.plane !== null) {
                    deviation = Math.max(deviation, Math.abs(this.plane.pointDistance(p)));
                }
                if (he.next() === null) {
                    break;
                }
                longest = Math.max(longest, Vector3Dd.distance(p, he.next()!.startingVertex.position));
                he = he.next()!;
            } while (he !== start);
        }
        this.minX = lowX;
        this.minY = lowY;
        this.minZ = lowZ;
        this.maxX = highX;
        this.maxY = highY;
        this.maxZ = highZ;
        this.planeDeviation = deviation;
        this.longestEdge = longest;
        this.dominantCoordinate =
            this.plane === null ? 0 : PolyhedralBoundedSolidFaceGeometry.dominantCoordinateOf(this.plane.getNormal());
    }

    /** Same choice as `_PolyhedralBoundedSolidFace.testPointInsideDetailed`. */
    private static dominantCoordinateOf(n: Vector3Dd): number {
        if (Math.abs(n.x()) >= Math.abs(n.y()) && Math.abs(n.x()) >= Math.abs(n.z())) {
            return 1;
        }
        if (Math.abs(n.y()) >= Math.abs(n.x()) && Math.abs(n.y()) >= Math.abs(n.z())) {
            return 2;
        }
        return 3;
    }

    /** One snapshot of `face`. */
    public static of(face: _PolyhedralBoundedSolidFace): PolyhedralBoundedSolidFaceGeometry;
    /** One snapshot per face, indexed like `solid.getPolygonsList()`. */
    public static of(solid: PolyhedralBoundedSolid): PolyhedralBoundedSolidFaceGeometry[];
    public static of(
        faceOrSolid: _PolyhedralBoundedSolidFace | PolyhedralBoundedSolid,
    ): PolyhedralBoundedSolidFaceGeometry | PolyhedralBoundedSolidFaceGeometry[] {
        if ("boundariesList" in faceOrSolid) {
            return new PolyhedralBoundedSolidFaceGeometry(faceOrSolid);
        }
        const solid = faceOrSolid;
        const n = solid.getPolygonsList().size();
        const out: PolyhedralBoundedSolidFaceGeometry[] = new Array<PolyhedralBoundedSolidFaceGeometry>(n);
        let i: number;

        for (i = 0; i < n; i++) {
            out[i] = new PolyhedralBoundedSolidFaceGeometry(solid.getPolygonsList().get(i)!);
        }
        return out;
    }

    /** The containing plane, or null for a degenerate face. */
    public getPlane(): InfinitePlane | null {
        return this.plane;
    }

    public getPlaneDeviation(): number {
        return this.planeDeviation;
    }

    /**
    Margin that covers every tolerance of the point-in-face test: vertex
    proximity (2 tolerance), edge proximity (tolerance) and the parametric
    slack of the segment test (VSDK.EPSILON times the edge length).
    */
    private pointTestMargin(tolerance: number): number {
        return 4.0 * tolerance + 2.0 * VSDK.EPSILON * this.longestEdge;
    }

    /**
    True when `point` is far enough from the face, in the two coordinates
    kept by the point-in-face projection, that
    `testPointInside(point, tolerance, getPlane())` must answer OUTSIDE.
    */
    public projectionExcludes(point: Vector3Dd, tolerance: number): boolean {
        const m = this.pointTestMargin(tolerance);
        const xOut = point.x() < this.minX - m || point.x() > this.maxX + m;
        const yOut = point.y() < this.minY - m || point.y() > this.maxY + m;
        const zOut = point.z() < this.minZ - m || point.z() > this.maxZ + m;

        switch (this.dominantCoordinate) {
            case 1:
                return yOut || zOut;
            case 2:
                return xOut || zOut;
            case 3:
                return xOut || yOut;
            default:
                return false;
        }
    }

    /**
    True when the boxes of the two faces, each grown by `margin`, do not
    overlap.
    */
    public boxSeparatedFrom(other: PolyhedralBoundedSolidFaceGeometry, margin: number): boolean {
        return (
            this.maxX + margin < other.minX - margin ||
            other.maxX + margin < this.minX - margin ||
            this.maxY + margin < other.minY - margin ||
            other.maxY + margin < this.minY - margin ||
            this.maxZ + margin < other.minZ - margin ||
            other.maxZ + margin < this.minZ - margin
        );
    }

    /**
    True when the box of the segment from `from` to `to` and the box of
    the face, each grown by `margin`, do not overlap.
    */
    public segmentSeparatedFrom(from: Vector3Dd, to: Vector3Dd, margin: number): boolean {
        return (
            Math.max(from.x(), to.x()) + margin < this.minX - margin ||
            Math.min(from.x(), to.x()) - margin > this.maxX + margin ||
            Math.max(from.y(), to.y()) + margin < this.minY - margin ||
            Math.min(from.y(), to.y()) - margin > this.maxY + margin ||
            Math.max(from.z(), to.z()) + margin < this.minZ - margin ||
            Math.min(from.z(), to.z()) - margin > this.maxZ + margin
        );
    }

    /**
    Margin to grow this face's box by so that it holds every point that a
    tolerance-`tolerance` point-in-face test against it can accept, also in
    the coordinate dropped by the projection (where a non-planar face can
    reach `planeDeviation` beyond its plane; the factor 2 bounds the
    sqrt(3) of a dominant normal component of at least 1/sqrt(3)).
    */
    public containmentMargin(tolerance: number): number {
        return this.pointTestMargin(tolerance) + 2.0 * (this.planeDeviation + tolerance);
    }
}
