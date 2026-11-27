//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
import { VSDK } from "../../../common/VSDK.js";
import { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import type { Geometry } from "../Geometry.js";
import type { GeometryBounds } from "../element/GeometryBounds.js";
import { GeometryContainment } from "../element/GeometryContainment.js";
import type { Ray } from "../element/Ray.js";
import { RayBoundary } from "../element/RayBoundary.js";
import { RayCrossing } from "../element/RayCrossing.js";
import { RayCrossingKind } from "../element/RayCrossingKind.js";
import { RayCrossingQuery } from "../element/RayCrossingQuery.js";
import type { RayCrossingSink } from "../element/RayCrossingSink.js";
import { RayCrossingSupport } from "../element/RayCrossingSupport.js";
import { RayFirstHitStatus } from "../element/RayFirstHitStatus.js";
import { RayHit } from "../element/RayHit.js";
import { RaySolidBoundary } from "../element/RaySolidBoundary.js";
import type { RaySolidClassification } from "../element/RaySolidClassification.js";
import { RaySolidQueryStatus } from "../element/RaySolidQueryStatus.js";
import type { PolyhedralBoundedSolid } from "./polyhedralBoundedSolid/PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidEulerConstructions } from "./polyhedralBoundedSolid/PolyhedralBoundedSolidEulerConstructions.js";
import { PolyhedralBoundedSolidEulerOperators } from "./polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.js";
import { PolyhedralBoundedSolidValidationEngine } from "./polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.js";
import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { Solid } from "./Solid.js";

/**
Interval [start, end] of the parameter of a line, whose ends are on the
given surfaces of the tube (`SURFACE_NONE` for an unbounded end).

Java's private static nested class `Tube.Interval`.
*/
class TubeInterval {
    public start: number;
    public end: number;
    public startSurface: number;
    public endSurface: number;

    public constructor(start: number, startSurface: number, end: number, endSurface: number) {
        this.start = start;
        this.end = end;
        this.startSurface = startSurface;
        this.endSurface = endSurface;
    }
}

/**
A `Tube` is a thick walled cylinder (a pipe): the solid between two coaxial
cylinders of radii `internalRadius` and `externalRadius`, with its axis along
Z, its base at z = 0 and its top at z = `height` (the same frame of a `Cone`
used as a cylinder).

Two degenerate cases are supported:
  - `internalRadius` = 0: a solid cylinder (no hole).
  - `height` = 0: a washer, a planar circular lamina with a circular hole on
    the plane z = 0. It has no interior: rays hit it (from both sides) but it
    contains no points, and its boundary representation is a lamina (two
    faces sharing their loops) of genus 1.

The interior of the tube along a ray (see `classifyRay`) is the slab
0 <= z <= height, intersected with the external cylinder and minus the
internal one: up to two intervals of the parameter of the ray (two when the
ray crosses the hole).
*/
export class Tube extends Solid {
    private static readonly DEFAULT_CIRCUMFERENCE_DIVISIONS = 36;
    private static readonly MIN_CIRCUMFERENCE_DIVISIONS = 3;

    /// Surfaces of the tube, used to tag the ends of the interior intervals
    private static readonly SURFACE_NONE = 0;
    private static readonly SURFACE_BOTTOM = 1;
    private static readonly SURFACE_TOP = 2;
    private static readonly SURFACE_EXTERNAL = 3;
    private static readonly SURFACE_INTERNAL = 4;

    private internalRadius: number;
    private externalRadius: number;
    private height: number;

    public constructor(internalRadius: number, externalRadius: number, height: number) {
        super();
        this.internalRadius = internalRadius;
        this.externalRadius = externalRadius;
        this.height = height;
        this.getControlSpecifications().push("double;internalRadius;[0, INFINITE)");
        this.getControlSpecifications().push("double;externalRadius;(0, INFINITE)");
        this.getControlSpecifications().push("double;height;[0, INFINITE)");
    }

    public getInternalRadius(): number {
        return this.internalRadius;
    }

    public getExternalRadius(): number {
        return this.externalRadius;
    }

    public getHeight(): number {
        return this.height;
    }

    public setInternalRadius(value: number): void {
        this.internalRadius = value;
    }

    public setExternalRadius(value: number): void {
        this.externalRadius = value;
    }

    public setHeight(value: number): void {
        this.height = value;
    }

    /**
    @return true if this tube has no height, so it is a washer (a planar
    lamina with a hole)
    */
    public isWasher(): boolean {
        return this.height <= 0.0;
    }

    public override copyGeometry(): Geometry | null {
        return new Tube(this.internalRadius, this.externalRadius, this.height);
    }

    /**
    @return true if the parameters describe a tube this class can work with
    */
    private hasValidParameters(): boolean {
        return (
            Double.isFinite(this.internalRadius) &&
            Double.isFinite(this.externalRadius) &&
            Double.isFinite(this.height) &&
            this.internalRadius >= 0 &&
            this.externalRadius > this.internalRadius &&
            this.height >= 0
        );
    }

    //= Interior of the tube along a ray =================================

    /**
    @return the interval of the line `origin + t * direction` inside the
    infinite cylinder of the given radius around Z, or null if the line does
    not cross it (or only touches it)
    */
    private static cylinderInterval(
        origin: Vector3Dd,
        direction: Vector3Dd,
        radius: number,
        surface: number,
    ): TubeInterval | null {
        const ox = origin.x();
        const oy = origin.y();
        const dx = direction.x();
        const dy = direction.y();
        const a = dx * dx + dy * dy;
        const c = ox * ox + oy * oy - radius * radius;

        if (a <= 0.0) {
            // Parallel to the axis: everything or nothing
            if (c < 0.0) {
                return new TubeInterval(
                    Double.NEGATIVE_INFINITY,
                    Tube.SURFACE_NONE,
                    Double.POSITIVE_INFINITY,
                    Tube.SURFACE_NONE,
                );
            }
            return null;
        }
        const b = 2.0 * (ox * dx + oy * dy);
        const discriminant = b * b - 4.0 * a * c;

        if (discriminant <= 0.0) {
            return null;
        }
        const root = JavaMath.sqrt(discriminant);
        // Numerically stable roots
        const q = b < 0.0 ? -0.5 * (b - root) : -0.5 * (b + root);
        let t0 = q / a;
        let t1 = q !== 0.0 ? c / q : -t0;

        if (t0 > t1) {
            const swap = t0;
            t0 = t1;
            t1 = swap;
        }
        return new TubeInterval(t0, surface, t1, surface);
    }

    /**
    @return the interval of the line inside the slab 0 <= z <= height, or
    null if it does not cross it
    */
    private slabInterval(origin: Vector3Dd, direction: Vector3Dd): TubeInterval | null {
        const oz = origin.z();
        const dz = direction.z();

        if (dz === 0.0) {
            if (oz > 0.0 && oz < this.height) {
                return new TubeInterval(
                    Double.NEGATIVE_INFINITY,
                    Tube.SURFACE_NONE,
                    Double.POSITIVE_INFINITY,
                    Tube.SURFACE_NONE,
                );
            }
            return null;
        }
        const tBottom = -oz / dz;
        const tTop = (this.height - oz) / dz;

        if (tBottom < tTop) {
            return new TubeInterval(tBottom, Tube.SURFACE_BOTTOM, tTop, Tube.SURFACE_TOP);
        }
        return new TubeInterval(tTop, Tube.SURFACE_TOP, tBottom, Tube.SURFACE_BOTTOM);
    }

    /**
    Calculates the parts of the line `origin + t * direction` (for every t)
    inside the tube, in increasing order. Empty intervals are discarded, so a
    line tangent to the tube has none.
    @param out receives up to two intervals
    @return number of intervals
    */
    private computeInteriorIntervals(origin: Vector3Dd, direction: Vector3Dd, out: (TubeInterval | null)[]): number {
        if (this.isWasher()) {
            return 0;
        }
        const slab = this.slabInterval(origin, direction);
        if (slab === null) {
            return 0;
        }
        const external = Tube.cylinderInterval(origin, direction, this.externalRadius, Tube.SURFACE_EXTERNAL);
        if (external === null) {
            return 0;
        }

        //- Slab intersected with the external cylinder --------------------
        const body = new TubeInterval(slab.start, slab.startSurface, slab.end, slab.endSurface);
        if (external.start > body.start) {
            body.start = external.start;
            body.startSurface = external.startSurface;
        }
        if (external.end < body.end) {
            body.end = external.end;
            body.endSurface = external.endSurface;
        }
        if (!(body.start < body.end)) {
            return 0;
        }

        //- Minus the internal cylinder ------------------------------------
        let hole: TubeInterval | null = null;
        if (this.internalRadius > 0.0) {
            hole = Tube.cylinderInterval(origin, direction, this.internalRadius, Tube.SURFACE_INTERNAL);
        }
        if (hole === null || hole.end <= body.start || hole.start >= body.end) {
            out[0] = body;
            return 1;
        }

        let n = 0;
        if (hole.start > body.start) {
            out[n++] = new TubeInterval(body.start, body.startSurface, hole.start, Tube.SURFACE_INTERNAL);
        }
        if (hole.end < body.end) {
            out[n++] = new TubeInterval(hole.end, Tube.SURFACE_INTERNAL, body.end, body.endSurface);
        }
        return n;
    }

    /**
    @return the parameter where the line crosses the washer (the plane z = 0
    between both radii), or NaN if it does not
    */
    private washerHit(origin: Vector3Dd, direction: Vector3Dd): number {
        const dz = direction.z();

        if (dz === 0.0) {
            return Double.NaN;
        }
        const t = -origin.z() / dz;
        const px = origin.x() + direction.x() * t;
        const py = origin.y() + direction.y() * t;
        const r2 = px * px + py * py;

        if (r2 > this.externalRadius * this.externalRadius || r2 < this.internalRadius * this.internalRadius) {
            return Double.NaN;
        }
        return t;
    }

    /**
    First point of the surface of the tube hit by a ray, after `minT`.
    @param surface one-element output holder receiving the surface hit
    @return the parameter of the hit, or NaN if there is none
    */
    private firstSurfaceHit(ray: Ray, minT: number, surface: number[]): number {
        const origin = ray.getOrigin();
        const direction = ray.getDirection();

        if (this.isWasher()) {
            const t = this.washerHit(origin, direction);
            if (!Double.isNaN(t) && t > minT) {
                surface[0] = direction.z() < 0 ? Tube.SURFACE_TOP : Tube.SURFACE_BOTTOM;
                return t;
            }
            return Double.NaN;
        }

        const intervals: (TubeInterval | null)[] = [null, null];
        const n = this.computeInteriorIntervals(origin, direction, intervals);
        for (let i = 0; i < n; i++) {
            const interval = intervals[i]!;
            if (interval.start > minT) {
                surface[0] = interval.startSurface;
                return interval.start;
            }
            if (interval.end > minT) {
                surface[0] = interval.endSurface;
                return interval.end;
            }
        }
        return Double.NaN;
    }

    //= Classic ray intersection interface ===============================

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.

    Java overloads `doIntersectionFirstHit(Ray inOutRay)` (returns the ray
    with the distance to the hit, or null if the ray does not hit current
    Tube) and `doIntersectionFirstHit(Ray inOutRay, RayHit outHit)`.
    */
    public doIntersectionFirstHit(inOutRay: Ray): Ray | null;
    public override doIntersectionFirstHit(inOutRay: Ray, outHit: RayHit): boolean;
    public override doIntersectionFirstHit(inOutRay: Ray, outHit: RayHit | null): boolean;
    public override doIntersectionFirstHit(inOutRay: Ray, outHit?: RayHit | null): Ray | null | boolean {
        if (outHit === undefined) {
            const hit = new RayHit();
            if (this.doIntersectionFirstHit(inOutRay, hit)) {
                return hit.getRay();
            }
            return null;
        }

        if (!this.hasValidParameters()) {
            return false;
        }
        const surface = [Tube.SURFACE_NONE];
        const t = this.firstSurfaceHit(inOutRay, VSDK.EPSILON, surface);

        if (Double.isNaN(t)) {
            return false;
        }
        if (outHit !== null) {
            if (outHit.shouldStoreRay() || outHit.needsAnySurfaceData()) {
                outHit.setRay(inOutRay.withT(t));
            } else {
                outHit.setHitDistance(t);
            }
            if (outHit.needsAnySurfaceData()) {
                this.fillSurfaceData(inOutRay, t, surface[0]!, outHit);
            }
        }
        return true;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    @param inRay
    @param inT
    @param outData
    */
    public override doExtraInformation(inRay: Ray, inT: number, outData: RayHit): void {
        if (outData === null) {
            return;
        }
        const p = inRay.getOrigin().add(inRay.getDirection().multiply(inT));
        this.fillSurfaceData(inRay, inT, this.surfaceAt(p, inRay.getDirection()), outData);
    }

    /**
    @return the surface of the tube nearest to a point of its boundary
    */
    private surfaceAt(p: Vector3Dd, rayDirection: Vector3Dd): number {
        if (this.isWasher()) {
            return rayDirection.z() < 0 ? Tube.SURFACE_TOP : Tube.SURFACE_BOTTOM;
        }
        const rho = JavaMath.sqrt(p.x() * p.x() + p.y() * p.y());
        const dBottom = JavaMath.abs(p.z());
        const dTop = JavaMath.abs(p.z() - this.height);
        const dExternal = JavaMath.abs(rho - this.externalRadius);
        const dInternal = this.internalRadius > 0 ? JavaMath.abs(rho - this.internalRadius) : Double.POSITIVE_INFINITY;
        let surface = Tube.SURFACE_BOTTOM;
        let best = dBottom;

        if (dTop < best) {
            best = dTop;
            surface = Tube.SURFACE_TOP;
        }
        if (dExternal < best) {
            best = dExternal;
            surface = Tube.SURFACE_EXTERNAL;
        }
        if (dInternal < best) {
            surface = Tube.SURFACE_INTERNAL;
        }
        return surface;
    }

    private fillSurfaceData(ray: Ray, t: number, surface: number, outData: RayHit): void {
        outData.beginDetailEvaluation();
        const p = ray.getOrigin().add(ray.getDirection().multiply(t));
        const rho = JavaMath.sqrt(p.x() * p.x() + p.y() * p.y());
        let angle = globalThis.Math.atan2(p.y(), p.x());
        if (angle < 0) {
            angle += 2 * JavaMath.PI;
        }
        const c = rho > 0 ? p.x() / rho : 1.0;
        const s = rho > 0 ? p.y() / rho : 0.0;
        let normal: Vector3Dd;

        switch (surface) {
            case Tube.SURFACE_TOP:
                normal = new Vector3Dd(0, 0, 1);
                break;
            case Tube.SURFACE_BOTTOM:
                normal = new Vector3Dd(0, 0, -1);
                break;
            case Tube.SURFACE_INTERNAL:
                normal = new Vector3Dd(-c, -s, 0);
                break;
            default:
                normal = new Vector3Dd(c, s, 0);
                break;
        }

        if (outData.needsPoint()) {
            outData.point = p;
            outData.markDetailAvailable(RayHit.DETAIL_POINT);
        }
        if (outData.needsNormal()) {
            outData.normal = normal;
            outData.markDetailAvailable(RayHit.DETAIL_NORMAL);
        }
        if (outData.needsTangent()) {
            outData.tangent = new Vector3Dd(-s, c, 0);
            outData.markDetailAvailable(RayHit.DETAIL_TANGENT);
        }
        if (outData.needsTextureCoordinates()) {
            outData.u = angle / (2 * JavaMath.PI);
            if (surface === Tube.SURFACE_TOP || surface === Tube.SURFACE_BOTTOM) {
                outData.v = (rho - this.internalRadius) / (this.externalRadius - this.internalRadius);
            } else {
                outData.v = this.height > 0 ? p.z() / this.height : 0;
            }
            outData.markDetailAvailable(RayHit.DETAIL_UV);
        }
    }

    //= Ray crossing and classification interface ========================

    /**
    @return null if the query and the ray can be processed, or the status
    explaining why they can not
    */
    private validateQuery(ray: Ray, query: RayCrossingQuery): RayFirstHitStatus | null {
        if (
            Double.isNaN(query.minT) ||
            Double.isNaN(query.maxT) ||
            query.minT > query.maxT ||
            !Double.isFinite(query.minimumChordSquared) ||
            query.minimumChordSquared < 0 ||
            Double.isNaN(query.minimumExteriorApproach)
        ) {
            return RayFirstHitStatus.InvalidInput;
        }
        if (query.minimumChordSquared !== 0 || query.minimumExteriorApproach !== Double.NEGATIVE_INFINITY) {
            return RayFirstHitStatus.Unsupported;
        }
        const o = ray.getOrigin();
        const d = ray.getDirection();
        if (
            !Double.isFinite(o.x()) ||
            !Double.isFinite(o.y()) ||
            !Double.isFinite(o.z()) ||
            !Double.isFinite(d.x()) ||
            !Double.isFinite(d.y()) ||
            !Double.isFinite(d.z()) ||
            (d.x() === 0 && d.y() === 0 && d.z() === 0)
        ) {
            return RayFirstHitStatus.InvalidInput;
        }
        if (!this.hasValidParameters()) {
            return RayFirstHitStatus.InvalidInput;
        }
        return null;
    }

    public override emitRayCrossings(ray: Ray, query: RayCrossingQuery, sink: RayCrossingSink): RayCrossingSupport {
        const invalid = this.validateQuery(ray, query);
        if (invalid === RayFirstHitStatus.Unsupported) {
            return RayCrossingSupport.Unsupported;
        }
        if (invalid !== null) {
            return RayCrossingSupport.InvalidInput;
        }

        if (this.isWasher()) {
            const t = this.washerHit(ray.getOrigin(), ray.getDirection());
            if (!Double.isNaN(t) && t >= query.minT && t <= query.maxT) {
                sink.crossing(new RayCrossing(t, this, RayCrossingKind.Tangent));
            }
            return RayCrossingSupport.Supported;
        }

        const intervals: (TubeInterval | null)[] = [null, null];
        const n = this.computeInteriorIntervals(ray.getOrigin(), ray.getDirection(), intervals);
        for (let i = 0; i < n; i++) {
            if (!Double.isFinite(intervals[i]!.start) || !Double.isFinite(intervals[i]!.end)) {
                return RayCrossingSupport.Indeterminate;
            }
        }
        for (let i = 0; i < n; i++) {
            const entry = intervals[i]!.start;
            const exit = intervals[i]!.end;
            if (entry >= query.minT && entry <= query.maxT) {
                sink.crossing(new RayCrossing(entry, this, RayCrossingKind.Entering));
            }
            if (exit >= query.minT && exit <= query.maxT) {
                sink.crossing(new RayCrossing(exit, this, RayCrossingKind.Leaving));
            }
        }
        return RayCrossingSupport.Supported;
    }

    public override queryFirstHit(
        ray: Ray,
        query: RayCrossingQuery,
        result: RayHit | null,
        boundary: RayBoundary | null = null,
    ): RayFirstHitStatus {
        if (result !== null) {
            result.clear();
        }
        if (boundary !== null) {
            boundary.assign(new RayBoundary());
        }
        const invalid = this.validateQuery(ray, query);
        if (invalid !== null) {
            return invalid;
        }

        // The first boundary at or after minT: the surface hit is searched
        // strictly after a parameter slightly smaller than minT
        const surface = [Tube.SURFACE_NONE];
        const t = this.firstSurfaceHit(ray, JavaMath.nextDown(query.minT), surface);

        if (Double.isNaN(t) || t > query.maxT) {
            return RayFirstHitStatus.Miss;
        }
        if (boundary !== null) {
            boundary.assign(new RayBoundary(t, this));
        }
        if (result !== null) {
            result.setHitDistance(t);
            if (result.shouldStoreRay()) {
                result.setRay(ray.withT(t));
            }
            if (result.needsAnySurfaceData()) {
                this.fillSurfaceData(ray, t, surface[0]!, result);
            }
        }
        return RayFirstHitStatus.Hit;
    }

    public override querySurfaceDetail(ray: Ray, boundary: RayBoundary, hit: RayHit | null): RayFirstHitStatus {
        if (hit !== null) {
            hit.clear();
        }
        if (hit === null || boundary.geometry !== this || !Double.isFinite(boundary.t)) {
            return RayFirstHitStatus.InvalidInput;
        }
        if (boundary.getInstanceCount() !== 0) {
            return RayFirstHitStatus.Unsupported;
        }
        const status = this.queryFirstHit(ray, new RayCrossingQuery(boundary.t, boundary.t), null);
        if (status !== RayFirstHitStatus.Hit) {
            return status;
        }
        hit.setHitDistance(boundary.t);
        if (hit.shouldStoreRay()) {
            hit.setRay(ray.withT(boundary.t));
        }
        this.doExtraInformation(ray, boundary.t, hit);
        if (
            boundary.normalReversed &&
            hit.needsNormal() &&
            (hit.getAvailableDetailMask() & RayHit.DETAIL_NORMAL) !== 0
        ) {
            hit.normal = hit.normal.multiply(-1);
        }
        return RayFirstHitStatus.Hit;
    }

    /**
    Classifies the ray against the interior of the tube, following the
    contract of `Geometry.classifyRay`: the first element of
    `result.insideSegments` tells if the ray starts (at `query.minT`) inside
    the tube, and each boundary toggles it. A washer has no interior.
    */
    public override classifyRay(
        ray: Ray,
        query: RayCrossingQuery,
        result: RaySolidClassification,
    ): RaySolidQueryStatus {
        result.clear();
        if (query.minT >= query.maxT) {
            return RaySolidQueryStatus.InvalidInput;
        }
        const invalid = this.validateQuery(ray, query);
        if (invalid === RayFirstHitStatus.Unsupported) {
            return RaySolidQueryStatus.Unsupported;
        }
        if (invalid !== null) {
            return RaySolidQueryStatus.InvalidInput;
        }

        const intervals: (TubeInterval | null)[] = [null, null];
        const n = this.computeInteriorIntervals(ray.getOrigin(), ray.getDirection(), intervals);
        let startsInside = false;

        for (let i = 0; i < n; i++) {
            if (query.minT > intervals[i]!.start && query.minT <= intervals[i]!.end) {
                startsInside = true;
            }
        }
        result.insideSegments.add(startsInside);
        for (let i = 0; i < n; i++) {
            const entry = intervals[i]!.start;
            const exit = intervals[i]!.end;
            if (entry >= query.minT && entry <= query.maxT) {
                result.boundaries.add(new RaySolidBoundary(entry, this));
                result.insideSegments.add(true);
            }
            if (exit >= query.minT && exit <= query.maxT) {
                result.boundaries.add(new RaySolidBoundary(exit, this));
                result.insideSegments.add(false);
            }
        }
        return RaySolidQueryStatus.Supported;
    }

    //= Containment ======================================================

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. Distances are measured perpendicular to each
    surface (caps, external and internal walls), so the tolerance is the same
    for all of them. Every point of a washer within the tolerance of it is
    `LIMIT`, as it has no interior.
    @param p point to classify, in the space of the tube
    @param distanceTolerance distance to the surface of the tube under which
    the point is classified as `LIMIT`
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    public override doContainmentTest(p: Vector3Dd, distanceTolerance: number): number {
        if (!this.hasValidParameters()) {
            return Tube.OUTSIDE;
        }
        const z = p.z();
        const rho = JavaMath.sqrt(p.x() * p.x() + p.y() * p.y());

        if (
            z < -distanceTolerance ||
            z > this.height + distanceTolerance ||
            rho > this.externalRadius + distanceTolerance ||
            rho < this.internalRadius - distanceTolerance
        ) {
            return Tube.OUTSIDE;
        }
        const insideHole = this.internalRadius > 0 && rho < this.internalRadius;
        if (insideHole && rho < this.internalRadius - distanceTolerance) {
            return Tube.OUTSIDE;
        }
        if (
            z > distanceTolerance &&
            z < this.height - distanceTolerance &&
            rho < this.externalRadius - distanceTolerance &&
            (this.internalRadius <= 0 || rho > this.internalRadius + distanceTolerance)
        ) {
            return Tube.INSIDE;
        }
        return Tube.LIMIT;
    }

    public override classifyPoint(p: Vector3Dd, distanceTolerance: number): GeometryContainment {
        if (
            !Double.isFinite(distanceTolerance) ||
            distanceTolerance < 0 ||
            !Double.isFinite(p.x()) ||
            !Double.isFinite(p.y()) ||
            !Double.isFinite(p.z())
        ) {
            return GeometryContainment.InvalidInput;
        }
        if (!this.hasValidParameters()) {
            return GeometryContainment.InvalidInput;
        }
        const c = this.doContainmentTest(p, distanceTolerance);
        if (c === Tube.INSIDE) {
            return GeometryContainment.Inside;
        }
        if (c === Tube.LIMIT) {
            return GeometryContainment.Boundary;
        }
        return GeometryContainment.Outside;
    }

    //= Bounds ===========================================================

    /** Finite bounds: |x|,|y| <= externalRadius and 0 <= z <= height. */
    public override getBounds(result: GeometryBounds): void {
        if (!this.hasValidParameters()) {
            result.setIndeterminate();
            return;
        }
        result.setFinite(
            new Vector3Dd(-this.externalRadius, -this.externalRadius, 0),
            new Vector3Dd(this.externalRadius, this.externalRadius, this.height),
        );
    }

    public override getMinMax(): Float64Array {
        const minmax = new Float64Array(6);

        minmax[0] = -this.externalRadius;
        minmax[1] = -this.externalRadius;
        minmax[2] = 0;
        minmax[3] = this.externalRadius;
        minmax[4] = this.externalRadius;
        minmax[5] = this.height;
        return minmax;
    }

    //= Boundary representation ==========================================

    /**
    Java overloads `exportToPolyhedralBoundedSolid()` and
    `exportToPolyhedralBoundedSolid(int circumferenceDivisions)`.
    @param circumferenceDivisions number of sides of the polygons
    approximating the circles (at least 3)
    @return the boundary representation of the tube
    */
    public override exportToPolyhedralBoundedSolid(): PolyhedralBoundedSolid;
    public override exportToPolyhedralBoundedSolid(circumferenceDivisions: number): PolyhedralBoundedSolid;
    public override exportToPolyhedralBoundedSolid(circumferenceDivisions?: number): PolyhedralBoundedSolid {
        if (circumferenceDivisions === undefined) {
            return this.buildPolyhedralBoundedSolid(Tube.DEFAULT_CIRCUMFERENCE_DIVISIONS);
        }
        return this.buildPolyhedralBoundedSolid(JavaMath.max(Tube.MIN_CIRCUMFERENCE_DIVISIONS, circumferenceDivisions));
    }

    /**
    Builds a planar circular lamina with a circular hole (a washer) on the
    plane z = `h`, using only Euler operators, following [MANT1988] 12.2
    (circular lamina, program 12.2) and the creation of a hole as an inner
    ring [MANT1988] 9.2.3 / 9.2.4:
      1. `createCircularLamina`: the external circle, as faces 1 and 2
         (`mvfs`, `smev` arc, `smef`).
      2. `smev` + `kemr`: a vertex of the internal circle is created hanging
         from the external one and the bridge edge is killed, so the vertex
         is left alone as an inner ring of face 1.
      3. `smev` arc: the rest of the internal circle, from that vertex.
      4. `smef`: the internal circle is closed, splitting from face 1 a new
         face (the disk of the hole).
      5. `kfmrh`: the disk is merged into face 2 as an inner ring, so both
         faces of the lamina have the hole.
    If `ri` is 0, the lamina has no hole (steps 2 to 5 are skipped).

    With `withQuadrilaterals`, each face of the lamina is further split so
    that the lamina is made of small planar polygons instead of two faces
    with a hole, as needed by programs that use the faces as patches:
      - with a hole: the two loops of each face are joined by an edge
        between the external vertex 1 and the internal vertex at its same
        angle (`lmekr`), and the face is split (`smef`) by an edge between
        every other external vertex and the internal vertex at its angle.
        Each face of the lamina becomes `n` quadrilaterals; the ones facing
        +Z are the faces 1 and 3 to n+1, and the ones facing -Z are 2 and
        n+2 to 2n. The internal vertex at the angle of the external vertex
        k+1 (at k*360/n degrees, k = 0 .. n-1) is n+1+((n-k) mod n).
      - without a hole (a disk): each face is split into a fan of `n`
        triangles towards a new vertex at its center (see `splitFaceIntoFan`);
        the two faces get distinct, coincident center vertices (n+1 for
        face 1 and n+2 for face 2).
    @param ri internal radius, or 0 for a disk
    @param re external radius
    @param h height of the plane of the lamina
    @param n number of sides of the circles (at least 3)
    @param withQuadrilaterals false (the default) for two faces with a hole,
    true to split them in quadrilaterals (triangles for a disk)
    @return the lamina; face 1 is the one facing +Z (it is the one to sweep)
    */
    public static createWasherLamina(
        ri: number,
        re: number,
        h: number,
        n: number,
        withQuadrilaterals = false,
    ): PolyhedralBoundedSolid {
        const solid: PolyhedralBoundedSolid = PolyhedralBoundedSolidEulerConstructions.createCircularLamina(
            0.0,
            0.0,
            re,
            h,
            n,
        );
        if (ri <= 0.0) {
            if (withQuadrilaterals) {
                Tube.splitFaceIntoFan(solid, 1, new Vector3Dd(0.0, 0.0, h));
                Tube.splitFaceIntoFan(solid, 2, new Vector3Dd(0.0, 0.0, h));
                PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
            }
            return solid;
        }

        const firstInternal = solid.getMaxVertexId() + 1;
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 1, firstInternal, new Vector3Dd(ri, 0.0, h));
        PolyhedralBoundedSolidEulerOperators.kemr(solid, 1, 1, 1, firstInternal, firstInternal, 1);
        // The internal circle goes clockwise (seen from +Z), against the
        // external one, so the disk it encloses is split as the new face
        PolyhedralBoundedSolidEulerConstructions.addArcToExistingFace(
            solid,
            1,
            firstInternal,
            0.0,
            0.0,
            ri,
            h,
            360.0,
            360.0 / n,
            n - 1,
        );
        const lastInternal = solid.getMaxVertexId();
        const diskFace = solid.getMaxFaceId() + 1;
        PolyhedralBoundedSolidEulerOperators.smef(solid, 1, lastInternal, firstInternal, diskFace);
        PolyhedralBoundedSolidEulerOperators.kfmrh(solid, 2, diskFace);
        if (withQuadrilaterals) {
            Tube.splitWasherIntoQuadrilaterals(solid, n);
        }
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
        return solid;
    }

    /**
    Splits face `faceId` of `solid`, a single loop polygon, into a fan of
    triangles towards a new vertex at `center` (`smev` from the first vertex
    of the loop, then `mef` around it). The face keeps one of the triangles
    and the others get new face ids, in the order of the loop.
    @return the id of the new center vertex, or 0 if the face does not
    exist or is not a polygon of at least three vertices
    */
    public static splitFaceIntoFan(solid: PolyhedralBoundedSolid, faceId: number, center: Vector3Dd): number {
        const face = solid.findFace(faceId);
        if (face === null || face.boundariesList.size() <= 0) {
            return 0;
        }

        const loop = face.boundariesList.get(0)!;
        const start = loop.boundaryStartHalfEdge;
        if (start === null) {
            return 0;
        }

        const ringVertexIds = new ArrayList<number>();
        let he: _PolyhedralBoundedSolidHalfEdge = start;
        do {
            ringVertexIds.add(he.startingVertex.id);
            he = he.next()!;
        } while (he !== start);

        if (ringVertexIds.size() < 3) {
            return 0;
        }

        const centerVertexId = solid.getMaxVertexId() + 1;
        PolyhedralBoundedSolidEulerOperators.smev(solid, faceId, ringVertexIds.get(0), centerVertexId, center);

        let i: number;
        for (i = 0; i < ringVertexIds.size() - 2; i++) {
            PolyhedralBoundedSolidEulerOperators.mef(
                solid,
                faceId,
                faceId,
                centerVertexId,
                ringVertexIds.get(i),
                ringVertexIds.get(i + 1),
                ringVertexIds.get(i + 2),
                solid.getMaxFaceId() + 1,
            );
        }

        PolyhedralBoundedSolidEulerOperators.mef(
            solid,
            faceId,
            faceId,
            centerVertexId,
            ringVertexIds.get(ringVertexIds.size() - 2),
            ringVertexIds.get(ringVertexIds.size() - 1),
            ringVertexIds.get(0),
            solid.getMaxFaceId() + 1,
        );
        return centerVertexId;
    }

    /**
    @return the id of the face among `candidateFaceIds` that has both
    vertices on one of its loops, or 0 if there is none
    */
    private static findFaceWithVertices(
        solid: PolyhedralBoundedSolid,
        candidateFaceIds: ArrayList<number>,
        vertexId1: number,
        vertexId2: number,
    ): number {
        let i: number;
        let j: number;

        for (i = 0; i < candidateFaceIds.size(); i++) {
            const face = solid.findFace(candidateFaceIds.get(i));
            if (face === null) {
                continue;
            }
            for (j = 0; j < face.boundariesList.size(); j++) {
                const loop = face.boundariesList.get(j)!;
                if (loop.firstHalfEdgeAtVertex(vertexId1) !== null && loop.firstHalfEdgeAtVertex(vertexId2) !== null) {
                    return face.id;
                }
            }
        }
        return 0;
    }

    /**
    Splits both faces of a washer lamina into quadrilaterals (see
    `createWasherLamina`).

    The external circle has the vertices 1..n (vertex k+1 at k*360/n degrees)
    and the internal one n+1..2n, clockwise: vertex n+1+j is at -j*360/n
    degrees, so the external vertex k+1 pairs with the internal vertex
    n+1+((n-k) mod n), at its same angle.
    */
    private static splitWasherIntoQuadrilaterals(solid: PolyhedralBoundedSolid, n: number): void {
        let nextFaceId = 3;
        let side: number;
        let k: number;

        for (side = 1; side <= 2; side++) {
            const sideFaces = new ArrayList<number>();
            sideFaces.add(side);
            const face = solid.findFace(side)!;
            PolyhedralBoundedSolidEulerOperators.lmekr(solid, face.findHalfEdge(1)!, face.findHalfEdge(n + 1)!);
            for (k = 1; k < n; k++) {
                const externalId = k + 1;
                const internalId = n + 1 + (n - k);
                const faceId = Tube.findFaceWithVertices(solid, sideFaces, externalId, internalId);
                PolyhedralBoundedSolidEulerOperators.smef(solid, faceId, externalId, internalId, nextFaceId);
                sideFaces.add(nextFaceId);
                nextFaceId++;
            }
        }
    }

    /**
    The tube is built as a translational sweep ([MANT1988] 12.3.1, program
    12.4) of a washer lamina (see `createWasherLamina`): its face 1, with
    both loops, is extruded `height` units along +Z. A washer is just the
    lamina.
    */
    private buildPolyhedralBoundedSolid(nsides: number): PolyhedralBoundedSolid {
        const solid = Tube.createWasherLamina(this.internalRadius, this.externalRadius, 0.0, nsides);

        if (!this.isWasher()) {
            const T = new Matrix4x4d().translation(0.0, 0.0, this.height);
            PolyhedralBoundedSolidEulerConstructions.translationalSweepExtrudeFace(solid, solid.findFace(1), T);
        }
        return solid;
    }
}
