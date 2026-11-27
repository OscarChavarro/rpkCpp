//= References:                                                             =
//= [APPE1967] Appel, Arthur. "The notion of quantitative invisibility and  =
//=          the machine rendering of solids". Proceedings, ACM National     =
//=          meeting 1967.                                                   =
//= [EDEL1990] Edelsbrunner, Mucke. "Simulation of Simplicity: a technique   =
//=          to cope with degenerate cases in geometric algorithms". ACM     =
//=          Transactions on Graphics, 1990.                                 =

import { Vector3Dd } from "../../../../common/linealAlgebra/Vector3Dd.js";
import { Geometry } from "../../Geometry.js";
import { Ray } from "../../element/Ray.js";
import { RayHit } from "../../element/RayHit.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import type { ToleranceContext } from "./PolyhedralBoundedSolidNumericPolicy.js";

/**
Robust geometric predicates over a {@link PolyhedralBoundedSolid}: discrete
in/out queries whose combinatorial answer is correct and consistent even at
degenerate configurations (a point exactly on a face boundary, a ray entering
through a shared edge or coplanar with a face).

<p>Degeneracies are the source of the residual errors in the Appel hidden-line
quantitative invisibility ([APPE1967]) on axis-aligned solids. A single finite
perturbation (jitter/tilt) is non-degenerate for some configurations and grazing
for others, so it cannot be robust. Following the spirit of Simulation of
Simplicity ([EDEL1990]) this class resolves degeneracies by choosing, per query,
a probe direction in <em>general position</em> with respect to the actual faces:
a face hit classified {@code LIMIT} (on the polygon boundary) flags that the
current probe grazes, and the query is retried with the next generic direction
until a clean one is found. Degeneracies are measure zero per direction, so a
clean probe is found almost immediately.

<p>Inputs and the solid geometry are in the solid's own (object/local)
coordinate system; callers that work in world space (e.g. the renderer through
{@code SimpleBody}) must transform the query point into object space first.

<p>This is a query layer; it does not modify the solid.

<p><b>Scope.</b> Provides the robust point-in-solid keystone
({@link #isPointInside}) and, built on it, the robust hidden-line quantitative
invisibility ({@link #quantitativeInvisibility}). The renderer uses the latter:
{@code PolyhedralBoundedSolid.computeQuantitativeInvisibility} delegates here, and
these methods reuse that solid's per-frame plane/AABB snapshot
({@code beginVisibilityQueries}) for performance.
*/
export class PolyhedralBoundedSolidPredicates {
    /// Generic, deliberately non-axis-aligned unit probe directions. Tried in
    /// order until one is in general position with the (often axis-aligned)
    /// faces of the queried solid. Their pairwise non-collinearity makes it
    /// astronomically unlikely that every one of them grazes the same query.
    private static readonly PROBE_DIRECTIONS: Vector3Dd[] = PolyhedralBoundedSolidPredicates.buildProbeDirections();

    private static buildProbeDirections(): Vector3Dd[] {
        const raw: Vector3Dd[] = [
            new Vector3Dd(0.3172, 0.549, 0.7725),
            new Vector3Dd(0.803, -0.1399, 0.5793),
            new Vector3Dd(-0.4211, 0.7022, 0.5735),
            new Vector3Dd(0.611, 0.4561, -0.647),
            new Vector3Dd(-0.5121, -0.3307, 0.7925),
            new Vector3Dd(0.2671, -0.8113, 0.5201),
            new Vector3Dd(-0.7402, 0.4517, 0.4983),
        ];
        const result: Vector3Dd[] = new Array<Vector3Dd>(raw.length);
        for (let i = 0; i < raw.length; i++) {
            result[i] = raw[i]!.normalized();
        }
        return result;
    }

    private constructor() {
        // Static utility class.
    }

    /**
    Robust point-in-solid test by ray-cast parity. A ray is cast from `point`
    along a generic direction and the number of times it strictly crosses the
    boundary is counted; an odd count means `point` is inside. If any crossing
    is degenerate (the hit lies on a face edge/vertex, classified LIMIT) the
    probe direction is grazing and the next generic direction is used.

    @param solid the solid to test against (object-space geometry)
    @param point the query point, in the solid's object space
    @return true if the point is strictly inside the solid
    */
    public static isPointInside(solid: PolyhedralBoundedSolid, point: Vector3Dd): boolean {
        const context = solid.queryToleranceContext();
        const bigEps = context.bigEpsilon();
        const maxT = context.modelScale() * 100.0 + 1.0;
        let lastCrossings = 0;
        for (let i = 0; i < PolyhedralBoundedSolidPredicates.PROBE_DIRECTIONS.length; i++) {
            const crossings = PolyhedralBoundedSolidPredicates.countBoundaryCrossingsToInfinity(
                solid,
                point,
                PolyhedralBoundedSolidPredicates.PROBE_DIRECTIONS[i]!,
                maxT,
                bigEps,
            );
            if (crossings >= 0) {
                return (crossings & 1) === 1;
            }
            lastCrossings = -(crossings + 1); // count gathered before grazing
        }
        // Every probe grazed (measure zero, essentially impossible). Fall back
        // to the parity of the final attempt's crossings before the grazing hit.
        return (lastCrossings & 1) === 1;
    }

    /**
    Classifies a point against the solid, as `Geometry.doContainmentTest`:
    `LIMIT` if it lies on a face (closer than the tolerance to the plane of
    the face, and inside the face polygon with that tolerance), otherwise
    `INSIDE` or `OUTSIDE` as decided by the robust `isPointInside`.

    (Ported from Java commit 402415f4, as a dependency of the free line
    piercing test of `HiddenLineRenderer`.)

    @param solid the solid to test against (object-space geometry)
    @param point the query point, in the solid's object space
    @param distanceTolerance distance to the boundary under which the point
    is classified as `LIMIT` (the numeric tolerance of the solid is used if
    it is larger)
    @return Geometry.INSIDE, Geometry.OUTSIDE or Geometry.LIMIT
    */
    public static classifyPoint(solid: PolyhedralBoundedSolid, point: Vector3Dd, distanceTolerance: number): number {
        const tolerance = Math.max(distanceTolerance, solid.queryToleranceContext().bigEpsilon());

        for (let i = 0; i < solid.getPolygonsList().size(); i++) {
            if (!solid.queryPointNearFace(point, i, tolerance)) {
                continue;
            }
            const face = solid.getPolygonsList().get(i)!;
            const plane = solid.cachedFacePlane(i);
            if (plane === null) {
                continue;
            }
            if (
                Math.abs(plane.pointDistance(point)) <= tolerance &&
                face.testPointInside(point, tolerance, plane) !== Geometry.OUTSIDE
            ) {
                return Geometry.LIMIT;
            }
        }
        return PolyhedralBoundedSolidPredicates.isPointInside(solid, point) ? Geometry.INSIDE : Geometry.OUTSIDE;
    }

    // Interval classification of a point on the line of sight.
    private static readonly OUTSIDE = 0;
    private static readonly INTERIOR = 1;
    private static readonly ON_SURFACE = 2;

    /**
    Quantitative invisibility [APPE1967] of `point` (which lies on the surface)
    as seen from `eye`: the number of front-facing surface elements strictly
    between the eye and the point — equivalently the number of times the STRAIGHT
    line of sight enters the solid INTERIOR before reaching the point.

    <p>Robust by construction, with no perturbation of the line of sight: the
    segment is split at every boundary crossing (captured tolerantly, so an
    edge/vertex graze becomes an interval boundary) and each interval midpoint is
    classified as strictly interior, strictly outside, or ON the surface (a
    midpoint lying on a face — the line of sight runs along the surface there,
    e.g. coplanar with a face). Only entries into a strictly-interior run count.
    A line of sight tangent to the silhouette merely touches the boundary (its
    neighbouring intervals are outside) and a coplanar one runs ON the surface,
    so neither fabricates an occluder — the two degeneracies that defeat a
    finite-perturbation jitter/tilt. The interior/outside decision delegates to
    the robust {@link #isPointInside}; the on-surface decision is an explicit
    in-plane / in-face test.

    <p>`point` is on the surface, so the target is pulled a small step back toward
    the eye to drop the faces incident to the point (a visible point then ends in
    free space; a truly occluded one stays inside the occluding material).

    @param solid the solid (object-space geometry)
    @param eye the observer, in the solid's object space
    @param point the surface point being classified, in object space
    @return the quantitative invisibility (0 = visible)
    */
    public static quantitativeInvisibility(solid: PolyhedralBoundedSolid, eye: Vector3Dd, point: Vector3Dd): number {
        const context = solid.queryToleranceContext();
        const eps = context.epsilon();
        let direction = point.subtract(eye);
        const distance = direction.length();
        if (distance <= eps) {
            return 0;
        }
        direction = direction.multiply(1.0 / distance);
        const reach = distance - distance * 1.0e-3;

        const crossings = PolyhedralBoundedSolidPredicates.collectBoundaryCrossings(
            solid,
            eye,
            direction,
            reach,
            context,
        );
        if (crossings.length === 0) {
            return 0; // the segment never meets the boundary: visible
        }

        let qi = 0;
        let previousInterior = false; // the eye is outside the solid
        let previousBound = 0.0;
        for (let i = 0; i <= crossings.length; i++) {
            const upper = i < crossings.length ? crossings[i]! : reach;
            if (upper - previousBound > eps) {
                const mid = eye.add(direction.multiply((previousBound + upper) * 0.5));
                const state = PolyhedralBoundedSolidPredicates.classifyOnSegment(solid, mid, context);
                const interior = state === PolyhedralBoundedSolidPredicates.INTERIOR;
                if (interior && !previousInterior) {
                    qi++;
                }
                previousInterior = interior;
            }
            previousBound = upper;
        }
        return qi;
    }

    /**
    Classifies a point known to lie on the line of sight as strictly INTERIOR,
    strictly OUTSIDE, or ON_SURFACE (on a face — the line of sight grazes the
    surface there). ON_SURFACE is decided first (a coplanar run must not be
    counted as interior); otherwise the robust point-in-solid test decides.
    */
    private static classifyOnSegment(
        solid: PolyhedralBoundedSolid,
        point: Vector3Dd,
        context: ToleranceContext,
    ): number {
        const bigEps = context.bigEpsilon();
        for (let i = 0; i < solid.getPolygonsList().size(); i++) {
            if (!solid.queryPointNearFace(point, i, bigEps)) {
                continue;
            }
            const face = solid.getPolygonsList().get(i)!;
            const plane = solid.cachedFacePlane(i);
            if (plane === null) {
                continue;
            }
            if (
                Math.abs(plane.pointDistance(point)) < bigEps &&
                face.testPointInside(point, bigEps, plane) !== Geometry.OUTSIDE
            ) {
                return PolyhedralBoundedSolidPredicates.ON_SURFACE;
            }
        }
        return PolyhedralBoundedSolidPredicates.isPointInside(solid, point)
            ? PolyhedralBoundedSolidPredicates.INTERIOR
            : PolyhedralBoundedSolidPredicates.OUTSIDE;
    }

    /**
    Sorted, deduplicated boundary-crossing distances of the segment
    (eye, eye + direction*reach): the distances at which the segment meets a face
    polygon (INSIDE or, tolerantly, LIMIT on its boundary). These delimit the
    intervals of uniform classification.
    */
    private static collectBoundaryCrossings(
        solid: PolyhedralBoundedSolid,
        eye: Vector3Dd,
        direction: Vector3Dd,
        reach: number,
        context: ToleranceContext,
    ): Float64Array {
        const ray = new Ray(eye, direction);
        const bigEps = context.bigEpsilon();
        const crossings: number[] = [];
        for (let i = 0; i < solid.getPolygonsList().size(); i++) {
            if (!solid.queryRayReachesFace(eye, direction.x(), direction.y(), direction.z(), reach, i, bigEps)) {
                continue;
            }
            const face = solid.getPolygonsList().get(i)!;
            const plane = solid.cachedFacePlane(i);
            if (plane === null) {
                continue;
            }
            const planeHit = new RayHit();
            if (!plane.doIntersectionFirstHit(ray, planeHit)) {
                continue;
            }
            let hit = planeHit.getRay()!;
            hit = hit.withDirection(hit.getDirection().normalized());
            const t = hit.getT();
            if (t <= bigEps || t >= reach) {
                continue;
            }
            const pi = hit.getOrigin().add(hit.getDirection().multiply(t));
            if (face.testPointInside(pi, bigEps, plane) === Geometry.OUTSIDE) {
                continue;
            }
            if (!PolyhedralBoundedSolidPredicates.containsApproxDistance(crossings, t, bigEps)) {
                crossings.push(t);
            }
        }
        const sorted = new Float64Array(crossings.length);
        for (let i = 0; i < crossings.length; i++) {
            sorted[i] = crossings[i]!;
        }
        sorted.sort();
        return sorted;
    }

    /**
    Counts the strict boundary crossings of the ray from `point` to infinity
    along `direction`. Returns the count, or a NEGATIVE value (the count so far
    encoded as {@code -(count+1)}) when a hit is classified LIMIT — signalling
    that the probe direction grazes a face and a different one should be used.
    */
    private static countBoundaryCrossingsToInfinity(
        solid: PolyhedralBoundedSolid,
        point: Vector3Dd,
        direction: Vector3Dd,
        maxT: number,
        bigEps: number,
    ): number {
        const ray = new Ray(point, direction);
        const crossings: number[] = [];
        for (let i = 0; i < solid.getPolygonsList().size(); i++) {
            if (!solid.queryRayReachesFace(point, direction.x(), direction.y(), direction.z(), maxT, i, bigEps)) {
                continue;
            }
            const face = solid.getPolygonsList().get(i)!;
            const plane = solid.cachedFacePlane(i);
            if (plane === null) {
                continue;
            }
            const planeHit = new RayHit();
            if (!plane.doIntersectionFirstHit(ray, planeHit)) {
                continue;
            }
            let hit = planeHit.getRay()!;
            hit = hit.withDirection(hit.getDirection().normalized());
            const t = hit.getT();
            if (t <= bigEps || t >= maxT) {
                continue;
            }
            const pi = hit.getOrigin().add(hit.getDirection().multiply(t));
            const classification = face.testPointInside(pi, bigEps, plane);
            if (classification === Geometry.LIMIT) {
                return -(crossings.length + 1); // grazing: ask for another probe
            }
            if (classification !== Geometry.INSIDE) {
                continue;
            }
            if (!PolyhedralBoundedSolidPredicates.containsApproxDistance(crossings, t, bigEps)) {
                crossings.push(t);
            }
        }
        return crossings.length;
    }

    private static containsApproxDistance(values: readonly number[], value: number, tolerance: number): boolean {
        for (let i = 0; i < values.length; i++) {
            if (Math.abs(values[i]! - value) < tolerance) {
                return true;
            }
        }
        return false;
    }
}
