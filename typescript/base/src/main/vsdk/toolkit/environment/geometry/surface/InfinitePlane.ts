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
            let aa = y.subtract(x);
            aa = aa.normalized();
            let bb = z.subtract(x);
            bb = bb.normalized();
            let n = aa.crossProduct(bb);
            n = n.normalized();
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
    /**
    `clone(other)` is Java's `clone(InfinitePlane)`; `clone()` is the one
    inherited from `Entity`.
    */
    public override clone(): unknown;
    public override clone(other: InfinitePlane): void;
    public override clone(other?: InfinitePlane): unknown {
        if (other === undefined) return super.clone();
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
    public override doIntersectionFirstHit(ray: Ray, hit: RayHit): boolean;
    public override doIntersectionFirstHit(ray: Ray, hit: RayHit | null): boolean;
    public override doIntersectionFirstHit(ray: Ray, hit?: RayHit | null): Ray | null | boolean {
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
    /**
    Por a given point `p`, calculates if it lies inside, outside or
    on surface with respect to current plane, taking the plane as an
    infinite halfspace, not as a surface.
    @return 0 if point is on the plane surface, 1 if point is outside or
    -1 if point is inside the plane - INSIDE, OUTSIDE or LIMIT constant value.
    Note that current interpretation of the plane is done as a semispace,
    where "outside" means the direction pointed by plane's normal.
    */
    public doContainmentTestHalfSpace(p: Vector3Dd, distanceTolerance: number): number {
        const num = this.a * p.x() + this.b * p.y() + this.c * p.z() + this.d;
        let op: number = InfinitePlane.LIMIT;
        if (num > distanceTolerance) {
            op = InfinitePlane.OUTSIDE;
        } else if (num < -distanceTolerance) {
            op = InfinitePlane.INSIDE;
        }
        return op;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest.
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    public override doContainmentTest(p: Vector3Dd, distanceTolerance: number): number {
        const num = this.a * p.x() + this.b * p.y() + this.c * p.z() + this.d;
        let op: number = InfinitePlane.LIMIT;
        if (num > distanceTolerance) {
            op = InfinitePlane.OUTSIDE;
        } else if (num < -distanceTolerance) {
            op = -InfinitePlane.INSIDE;
        }
        return op;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    */
    public override doExtraInformation(inRay: Ray, inT: number, outData: RayHit): void {
        if ((outData as RayHit | null) === null) return;
        outData.beginDetailEvaluation();
        outData.point = new Vector3Dd(
            inRay.getOrigin().x() + inT * inRay.getDirection().x(),
            inRay.getOrigin().y() + inT * inRay.getDirection().y(),
            inRay.getOrigin().z() + inT * inRay.getDirection().z(),
        );
        outData.normal = this.surfaceNormal();
        if (outData.needsPoint()) outData.markDetailAvailable(RayHit.DETAIL_POINT);
        if (outData.needsNormal()) outData.markDetailAvailable(RayHit.DETAIL_NORMAL);
    }

    /**
    TODO: Current returned values are not always valid!
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    public override getMinMax(): number[] {
        const minmax: number[] = [0, 0, 0, 0, 0, 0];
        for (let i = 0; i < 3; i++) {
            minmax[i] = -Double.MAX_VALUE;
        }
        for (let i = 3; i < 6; i++) {
            minmax[i] = Double.MAX_VALUE;
        }
        return minmax;
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
    /**
    Given a plane normal and a point in the plane, this method updates
    current plane to fit that spec. As in Java, `d` is computed from the
    given (not normalized) normal.
    */
    public setFromPointNormal(p: Vector3Dd, n: Vector3Dd): void {
        this.setNormal(n);
        this.setD(-(n.x() * p.x() + n.y() * p.y() + n.z() * p.z()));
    }
    public pointDistance(p: Vector3Dd) {
        return this.a * p.x() + this.b * p.y() + this.c * p.z() + this.d;
    }
    public projectPoint(p: Vector3Dd): Vector3Dd {
        const distance = this.pointDistance(p);
        let n = new Vector3Dd(this.a, this.b, this.c);
        n = n.normalized();
        n = n.multiply(distance);
        return p.subtract(n);
    }
    public mirrorPoint(p: Vector3Dd): Vector3Dd {
        const distance = this.pointDistance(p);
        let n = new Vector3Dd(this.a, this.b, this.c);
        n = n.normalized();
        n = n.multiply(2 * distance);
        return p.subtract(n);
    }
    /**
    Returns `true` if `this` plane is overlaping with `other` plane.
    Note that two planes are overlaping if they are both coplanar and
    at the same distance from the origin.
    */
    public overlapsWith(other: InfinitePlane, tolerance: number): boolean {
        let a1 = this.a;
        let b1 = this.b;
        let c1 = this.c;
        let d1 = this.d;
        let a2 = other.a;
        let b2 = other.b;
        let c2 = other.c;
        let d2 = other.d;
        const n1 = new Vector3Dd(a1, b1, c1);
        const n2 = new Vector3Dd(a2, b2, c2);
        const l1 = n1.length();
        const l2 = n2.length();
        a1 /= l1;
        b1 /= l1;
        c1 /= l1;
        d1 /= l1;
        a2 /= l2;
        b2 /= l2;
        c2 /= l2;
        d2 /= l2;
        return (
            Math.abs(a2 - a1) <= tolerance &&
            Math.abs(b2 - b1) <= tolerance &&
            Math.abs(c2 - c1) <= tolerance &&
            Math.abs(d2 - d1) <= tolerance
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
