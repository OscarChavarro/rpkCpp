import { HalfSpace } from "./HalfSpace.js";
import { QuadMesh } from "./QuadMesh.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { VSDK } from "../../../common/VSDK.js";
import { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import { RayBoundary } from "../element/RayBoundary.js";
import { RayCrossing } from "../element/RayCrossing.js";
import { RayCrossingKind } from "../element/RayCrossingKind.js";
import { RayCrossingQuery } from "../element/RayCrossingQuery.js";
import type { RayCrossingSink } from "../element/RayCrossingSink.js";
import { RayCrossingSupport } from "../element/RayCrossingSupport.js";
import { RayFirstHitStatus } from "../element/RayFirstHitStatus.js";
import type { GeometryBounds } from "../element/GeometryBounds.js";
import type { Geometry } from "../Geometry.js";
import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
export class InfinitePlane extends HalfSpace<Ray, RayHit> {
    private a: number;
    private b: number;
    private c: number;
    private d: number;
    public constructor(a: InfinitePlane);
    public constructor(a: number, b: number, c: number, d: number);
    public constructor(a: Vector3Dd, b: Vector3Dd);
    public constructor(a: Vector3Dd, b: Vector3Dd, c: Vector3Dd);
    public constructor(
        x: InfinitePlane | Vector3Dd | number,
        y?: Vector3Dd | number,
        z?: Vector3Dd | number,
        w?: number,
    ) {
        super();
        if (x instanceof InfinitePlane) {
            this.a = x.a;
            this.b = x.b;
            this.c = x.c;
            this.d = x.d;
        } else if (x instanceof Vector3Dd && y instanceof Vector3Dd && z instanceof Vector3Dd) {
            const n = y.subtract(x).normalized().crossProduct(z.subtract(x).normalized()).normalized();
            this.a = n.x();
            this.b = n.y();
            this.c = n.z();
            this.d = -n.dotProduct(x);
        } else if (x instanceof Vector3Dd && y instanceof Vector3Dd) {
            const n = x.normalized();
            this.a = n.x();
            this.b = n.y();
            this.c = n.z();
            this.d = -n.dotProduct(y);
        } else {
            this.a = x as number;
            this.b = y as number;
            this.c = z as number;
            this.d = w!;
        }
    }
    public override clone(other?: InfinitePlane): InfinitePlane | void {
        if (other === undefined) return new InfinitePlane(this);
        this.a = other.a;
        this.b = other.b;
        this.c = other.c;
        this.d = other.d;
    }
    public override copyGeometry(): Geometry | null {
        return new InfinitePlane(this);
    }

    public override getBounds(result: GeometryBounds): void {
        if (
            !Double.isFinite(this.a) ||
            !Double.isFinite(this.b) ||
            !Double.isFinite(this.c) ||
            !Double.isFinite(this.d) ||
            (this.a === 0 && this.b === 0 && this.c === 0)
        )
            result.setIndeterminate();
        else result.setUnbounded();
    }

    private evaluateRay(ray: Ray, query: RayCrossingQuery, depth: number[]): RayFirstHitStatus {
        const origin = ray.getOrigin();
        const direction = ray.getDirection();
        const a = this.a,
            b = this.b,
            c = this.c,
            d = this.d;
        if (
            !Double.isFinite(a) ||
            !Double.isFinite(b) ||
            !Double.isFinite(c) ||
            !Double.isFinite(d) ||
            (a === 0 && b === 0 && c === 0) ||
            !Double.isFinite(origin.x()) ||
            !Double.isFinite(origin.y()) ||
            !Double.isFinite(origin.z()) ||
            !Double.isFinite(direction.x()) ||
            !Double.isFinite(direction.y()) ||
            !Double.isFinite(direction.z()) ||
            (direction.x() === 0 && direction.y() === 0 && direction.z() === 0) ||
            Double.isNaN(query.minT) ||
            Double.isNaN(query.maxT) ||
            query.minT > query.maxT ||
            !Double.isFinite(query.minimumChordSquared) ||
            query.minimumChordSquared < 0 ||
            Double.isNaN(query.minimumExteriorApproach)
        )
            return RayFirstHitStatus.InvalidInput;
        if (query.minimumChordSquared !== 0 || query.minimumExteriorApproach !== Double.NEGATIVE_INFINITY)
            return RayFirstHitStatus.Unsupported;
        const normal = new Vector3Dd(a, b, c);
        const squared = normal.dotProduct(normal);
        const numerator = -(normal.dotProduct(origin) + d);
        const denominator = normal.dotProduct(direction);
        if (!Double.isFinite(squared) || squared === 0 || !Double.isFinite(numerator) || !Double.isFinite(denominator))
            return RayFirstHitStatus.Indeterminate;
        if (
            denominator === 0 &&
            ((a !== 0 && direction.x() !== 0 && a * direction.x() === 0) ||
                (b !== 0 && direction.y() !== 0 && b * direction.y() === 0) ||
                (c !== 0 && direction.z() !== 0 && c * direction.z() === 0))
        )
            return RayFirstHitStatus.Indeterminate;
        const status = InfinitePlane.intersectionDepth(numerator, denominator, 0, query.minT, query.maxT, true, depth);
        if (status !== RayFirstHitStatus.Hit) return status;
        const point = origin.add(direction.multiply(depth[0]!));
        if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) || !Double.isFinite(point.z()))
            return RayFirstHitStatus.Indeterminate;
        return RayFirstHitStatus.Hit;
    }

    public override emitRayCrossings(ray: Ray, query: RayCrossingQuery, sink: RayCrossingSink): RayCrossingSupport {
        const depth = [0];
        const status = this.evaluateRay(ray, query, depth);
        if (status === RayFirstHitStatus.Unsupported) return RayCrossingSupport.Unsupported;
        if (status === RayFirstHitStatus.InvalidInput) return RayCrossingSupport.InvalidInput;
        if (status === RayFirstHitStatus.Indeterminate) return RayCrossingSupport.Indeterminate;
        if (status === RayFirstHitStatus.Hit)
            sink.crossing(new RayCrossing(depth[0]!, this, RayCrossingKind.Unclassified));
        return RayCrossingSupport.Supported;
    }

    public override queryFirstHit(
        ray: Ray,
        query: RayCrossingQuery,
        result: RayHit | null,
        boundary: RayBoundary | null = null,
    ): RayFirstHitStatus {
        if (result) result.clear();
        if (boundary) boundary.assign(new RayBoundary());
        const depth = [0];
        const status = this.evaluateRay(ray, query, depth);
        if (status !== RayFirstHitStatus.Hit) return status;
        if (boundary) boundary.assign(new RayBoundary(depth[0]!, this));
        if (result) {
            result.setHitDistance(depth[0]!);
            if (result.shouldStoreRay()) result.setRay(ray.withT(depth[0]!));
            if (result.needsAnySurfaceData()) this.doExtraInformation(ray, depth[0]!, result);
        }
        return RayFirstHitStatus.Hit;
    }

    public override querySurfaceDetail(ray: Ray, boundary: RayBoundary, hit: RayHit | null): RayFirstHitStatus {
        if (hit) hit.clear();
        if (!hit || boundary.geometry !== this || !Double.isFinite(boundary.t)) return RayFirstHitStatus.InvalidInput;
        if (boundary.getInstanceCount() !== 0) return RayFirstHitStatus.Unsupported;
        const status = this.queryFirstHit(ray, new RayCrossingQuery(boundary.t, boundary.t), null);
        if (status !== RayFirstHitStatus.Hit) return status;
        hit.setHitDistance(boundary.t);
        if (hit.shouldStoreRay()) hit.setRay(ray.withT(boundary.t));
        this.doExtraInformation(ray, boundary.t, hit);
        if (boundary.normalReversed && hit.needsNormal() && hit.getAvailableDetailMask() & RayHit.DETAIL_NORMAL)
            hit.normal = hit.normal.multiply(-1);
        return RayFirstHitStatus.Hit;
    }

    private surfaceNormal(): Vector3Dd {
        const normal = new Vector3Dd(this.a, this.b, this.c);
        const squared = normal.dotProduct(normal);
        if (squared > 0 && squared < VSDK.EPSILON) return normal.multiply(1.0 / JavaMath.sqrt(squared));
        return normal.normalized();
    }

    // Intersect a plane from its precomputed signed numerator and directional
    // denominator. The caller supplies the parallel and interval policies.
    public static intersectionDepth(
        numerator: number,
        denominator: number,
        parallelTolerance: number,
        minT: number,
        maxT: number,
        checkArithmetic: boolean,
        depth: number[],
    ): RayFirstHitStatus {
        if (
            checkArithmetic &&
            (!Double.isFinite(numerator) ||
                !Double.isFinite(denominator) ||
                !Double.isFinite(parallelTolerance) ||
                parallelTolerance < 0 ||
                Double.isNaN(minT) ||
                Double.isNaN(maxT) ||
                minT > maxT)
        )
            return RayFirstHitStatus.InvalidInput;
        const parallel =
            parallelTolerance === 0
                ? denominator === 0
                : denominator < parallelTolerance && denominator > -parallelTolerance;
        if (parallel)
            return checkArithmetic && numerator === 0 ? RayFirstHitStatus.Indeterminate : RayFirstHitStatus.Miss;
        depth[0] = numerator / denominator;
        if (checkArithmetic && !Double.isFinite(depth[0])) return RayFirstHitStatus.Indeterminate;
        return depth[0] >= minT && depth[0] <= maxT ? RayFirstHitStatus.Hit : RayFirstHitStatus.Miss;
    }

    // Raw coefficients preserve the equation scale; getNormal() is normalized.
    public getCoefficients(): Vector3Dd {
        return new Vector3Dd(this.a, this.b, this.c);
    }
    public getConstantCoefficient(): number {
        return this.d;
    }
    public setCoefficients(normal: Vector3Dd, constant: number): void {
        this.a = normal.x();
        this.b = normal.y();
        this.c = normal.z();
        this.d = constant;
    }

    public doIntersectionFirstHit(ray: Ray): Ray | null;
    public doIntersectionFirstHit(ray: Ray, hit: RayHit): boolean;
    public doIntersectionFirstHit(ray: Ray, hit?: RayHit): Ray | null | boolean {
        if (hit === undefined) {
            const denominator =
                this.a * ray.getDirection().x() + this.b * ray.getDirection().y() + this.c * ray.getDirection().z();
            if (Math.abs(denominator) < VSDK.EPSILON) return null;
            const t =
                -(this.a * ray.getOrigin().x() + this.b * ray.getOrigin().y() + this.c * ray.getOrigin().z() + this.d) /
                denominator;

            if (t < 0) return null;

            return ray.withT(t);
        }

        const r = this.doIntersectionFirstHit(ray);
        if (r === null) {
            return false;
        }
        if (hit !== null) {
            hit.setRay(r);
            this.doExtraInformation(r, r.getT(), hit);
        }
        return true;
    }
    public doIntersectionWithNegative(ray: Ray): Ray | null {
        const denominator =
            this.a * ray.getDirection().x() + this.b * ray.getDirection().y() + this.c * ray.getDirection().z();
        if (Math.abs(denominator) < VSDK.EPSILON) {
            const r = new Ray(ray.getOrigin(), ray.getDirection().multiply(-1));
            const hit = this.doIntersectionFirstHit(r);
            if (hit !== null) {
                return ray.withT(-hit.getT());
            } else {
                return null;
            }
        }
        const t =
            -(this.a * ray.getOrigin().x() + this.b * ray.getOrigin().y() + this.c * ray.getOrigin().z() + this.d) /
            denominator;

        return ray.withT(t);
    }
    public doContainmentTestHalfSpace(p: Vector3Dd, t: number) {
        const q = this.pointDistance(p);
        return q > t ? InfinitePlane.OUTSIDE : q < -t ? InfinitePlane.INSIDE : InfinitePlane.LIMIT;
    }
    public override doContainmentTest(p: Vector3Dd, t: number) {
        const q = this.pointDistance(p);
        return q > t ? InfinitePlane.OUTSIDE : q < -t ? -InfinitePlane.INSIDE : InfinitePlane.LIMIT;
    }
    public override doExtraInformation(ray: Ray, t: number, out: RayHit) {
        if (!out) return;
        out.beginDetailEvaluation();
        out.point = ray.getOrigin().add(ray.getDirection().multiply(t));
        out.normal = this.surfaceNormal();
        if (out.needsPoint()) out.markDetailAvailable(RayHit.DETAIL_POINT);
        if (out.needsNormal()) out.markDetailAvailable(RayHit.DETAIL_NORMAL);
    }
    public getMinMax() {
        return new Float64Array([-Infinity, -Infinity, -Infinity, Infinity, Infinity, Infinity]);
    }
    /**
    A finite part of the plane, to present it: a square of side `size`
    divided into `tiles` x `tiles` cells, centered near a given point (i.e.
    the point of the plane viewed by a camera). The center is the projection
    of the point onto the plane, moved to the nearest corner of the cells of
    a grid fixed on the plane (with a corner at the point of the plane
    closest to the origin), so the lines of the square stay in the same
    places of the plane as the point moves. The sides follow two orthonormal
    directions (u, v) on the plane, with (u, v, normal) right handed: u is
    perpendicular to z (to x if the normal is almost along z).
    @param near point near which the square is centered
    @param size side of the square
    @param tiles cells along each side of the square, at least 1
    @return a mesh of `tiles` x `tiles` quadrilaterals, or null if the plane
    is degenerated (zero normal)
    */
    public exportToQuadMesh(near: Vector3Dd, size: number, tiles: number): QuadMesh | null {
        const length = Math.sqrt(this.a * this.a + this.b * this.b + this.c * this.c);
        if (!(length > 0) || tiles < 1) {
            return null;
        }
        const n = this.getNormal();
        const origin = n.multiply(-this.d / length);
        const reference = Math.abs(n.z()) < 0.9 ? new Vector3Dd(0, 0, 1) : new Vector3Dd(1, 0, 0);
        const u = reference.crossProduct(n).normalized();
        const v = n.crossProduct(u).normalized();
        const cell = size / tiles;
        const offset = this.projectPoint(near).subtract(origin);
        const cu = JavaMath.rint(offset.dotProduct(u) / cell) * cell;
        const cv = JavaMath.rint(offset.dotProduct(v) / cell) * cell;
        const corner = origin.add(u.multiply(cu - size / 2)).add(v.multiply(cv - size / 2));
        const vertices = tiles + 1;
        const mesh = new QuadMesh();
        let i: number;
        let j: number;
        let index: number;

        mesh.initVertexPositionsArray(vertices * vertices);
        const positions = mesh.getVertexPositions()!;
        for (i = 0; i < vertices; i++) {
            for (j = 0; j < vertices; j++) {
                const p = corner.add(u.multiply(i * cell)).add(v.multiply(j * cell));
                index = i * vertices + j;
                positions[3 * index] = p.x();
                positions[3 * index + 1] = p.y();
                positions[3 * index + 2] = p.z();
            }
        }
        mesh.initQuadArrays(tiles * tiles);
        index = 0;
        for (i = 0; i < tiles; i++) {
            for (j = 0; j < tiles; j++) {
                // Counterclockwise seen from the normal
                mesh.setQuadAt(
                    index,
                    i * vertices + j,
                    (i + 1) * vertices + j,
                    (i + 1) * vertices + j + 1,
                    i * vertices + j + 1,
                );
                index++;
            }
        }
        return mesh;
    }

    public getNormal() {
        return new Vector3Dd(this.a, this.b, this.c).normalized();
    }
    public getD() {
        return this.d;
    }
    public setNormal(x: Vector3Dd) {
        const n = x.normalized();
        this.a = n.x();
        this.b = n.y();
        this.c = n.z();
    }
    public setD(x: number) {
        this.d = x;
    }
    public setFromPointNormal(p: Vector3Dd, n: Vector3Dd) {
        this.setNormal(n);
        this.d = -(this.a * p.x() + this.b * p.y() + this.c * p.z());
    }
    public pointDistance(p: Vector3Dd) {
        return this.a * p.x() + this.b * p.y() + this.c * p.z() + this.d;
    }
    public projectPoint(p: Vector3Dd) {
        return p.subtract(this.getNormal().multiply(this.pointDistance(p)));
    }
    public mirrorPoint(p: Vector3Dd) {
        return p.subtract(this.getNormal().multiply(2 * this.pointDistance(p)));
    }
    public overlapsWith(o: InfinitePlane, t: number) {
        const l1 = Math.hypot(this.a, this.b, this.c),
            l2 = Math.hypot(o.a, o.b, o.c);
        return (
            Math.abs(o.a / l2 - this.a / l1) <= t &&
            Math.abs(o.b / l2 - this.b / l1) <= t &&
            Math.abs(o.c / l2 - this.c / l1) <= t &&
            Math.abs(o.d / l2 - this.d / l1) <= t
        );
    }
    public getA() {
        return this.a;
    }
    public setA(x: number) {
        this.a = x;
    }
    public getB() {
        return this.b;
    }
    public setB(x: number) {
        this.b = x;
    }
    public getC() {
        return this.c;
    }
    public setC(x: number) {
        this.c = x;
    }
    public override toString() {
        return `InfinitePlane: N=<${VSDK.formatDouble(this.a)}, ${VSDK.formatDouble(this.b)}, ${VSDK.formatDouble(this.c)}>, D=${VSDK.formatDouble(this.d)}`;
    }
}
