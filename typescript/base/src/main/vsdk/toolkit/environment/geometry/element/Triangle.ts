import { FundamentalEntity } from "../../../common/FundamentalEntity.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { VSDK } from "../../../common/VSDK.js";
import { Containment } from "../../../processing/Containment.js";
import { Ray } from "./Ray.js";
import { Intersection } from "./Intersection.js";
import type { RayCrossingQuery } from "./RayCrossingQuery.js";
import { RayFirstHitStatus } from "./RayFirstHitStatus.js";
import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
export class Triangle extends FundamentalEntity {
    private normal = new Vector3Dd();
    public constructor(
        private point0 = 0,
        private point1 = 0,
        private point2 = 0,
    ) {
        super();
    }
    public getPoint0() {
        return this.point0;
    }
    public getPoint1() {
        return this.point1;
    }
    public getPoint2() {
        return this.point2;
    }
    public setPoint0(value: number) {
        this.point0 = value;
    }
    public setPoint1(value: number) {
        this.point1 = value;
    }
    public setPoint2(value: number) {
        this.point2 = value;
    }
    public getNormal() {
        return this.normal;
    }
    public setNormal(x: Vector3Dd) {
        this.normal = x;
    }
    public static queryIntersection(
        ray: Ray,
        v0: Vector3Dd,
        v1: Vector3Dd,
        v2: Vector3Dd,
        query: RayCrossingQuery,
        t: number[],
        u: number[],
        v: number[],
    ): RayFirstHitStatus {
        const p = ray.getOrigin();
        const d = ray.getDirection();
        if (
            Double.isNaN(query.minT) ||
            Double.isNaN(query.maxT) ||
            query.minT > query.maxT ||
            !Double.isFinite(query.minimumChordSquared) ||
            query.minimumChordSquared < 0 ||
            Double.isNaN(query.minimumExteriorApproach) ||
            !Double.isFinite(p.x()) ||
            !Double.isFinite(p.y()) ||
            !Double.isFinite(p.z()) ||
            !Double.isFinite(d.x()) ||
            !Double.isFinite(d.y()) ||
            !Double.isFinite(d.z()) ||
            (d.x() === 0 && d.y() === 0 && d.z() === 0) ||
            !Double.isFinite(v0.x()) ||
            !Double.isFinite(v0.y()) ||
            !Double.isFinite(v0.z()) ||
            !Double.isFinite(v1.x()) ||
            !Double.isFinite(v1.y()) ||
            !Double.isFinite(v1.z()) ||
            !Double.isFinite(v2.x()) ||
            !Double.isFinite(v2.y()) ||
            !Double.isFinite(v2.z())
        )
            return RayFirstHitStatus.InvalidInput;
        if (query.minimumChordSquared !== 0 || query.minimumExteriorApproach !== Double.NEGATIVE_INFINITY)
            return RayFirstHitStatus.Unsupported;
        const lengthSquared = d.dotProduct(d);
        if (!Double.isFinite(lengthSquared) || lengthSquared === 0) return RayFirstHitStatus.Indeterminate;
        return Triangle.intersectRayCore(ray, v0, v1, v2, query.minT, query.maxT, 0, true, true, t, u, v);
    }

    public static doIntersectionWithTriangle(
        ray: Ray,
        v0: Vector3Dd,
        v1: Vector3Dd,
        v2: Vector3Dd,
    ): Intersection | null {
        const t = [0],
            u = [0],
            v = [0];
        if (!Triangle.intersectRay(ray, v0, v1, v2, t, u, v)) return null;

        const point = ray.getOrigin().add(ray.getDirection().multiply(t[0]!));
        const normal = v1.subtract(v0).crossProduct(v2.subtract(v0)).normalized();
        return new Intersection(t[0]!, point, normal);
    }

    public static supportingPlane(
        p0: Vector3Dd,
        p1: Vector3Dd,
        p2: Vector3Dd,
        minimumNormalLength: number,
        normal: Vector3Dd[],
        distance: number[],
    ): boolean {
        normal[0] = p0.subtract(p1).crossProduct(p2.subtract(p1));
        const length = normal[0].length();
        if (!Double.isFinite(length) || length === 0 || length < minimumNormalLength) return false;
        normal[0] = normal[0].multiply(1.0 / length);
        distance[0] = -normal[0].dotProduct(p0);
        return true;
    }

    public static dominantProjection(normal: Vector3Dd): Triangle.ProjectionAxis {
        const x = JavaMath.abs(normal.x()),
            y = JavaMath.abs(normal.y());
        const z = JavaMath.abs(normal.z());
        return x > y
            ? x > z
                ? Triangle.ProjectionAxis.X
                : Triangle.ProjectionAxis.Z
            : y > z
              ? Triangle.ProjectionAxis.Y
              : Triangle.ProjectionAxis.Z;
    }

    public static requiresProjectionSwap(
        p0: Vector3Dd,
        p1: Vector3Dd,
        p2: Vector3Dd,
        axis: Triangle.ProjectionAxis,
    ): boolean {
        if (axis === Triangle.ProjectionAxis.X)
            return (p1.y() - p2.y()) * (p1.z() - p0.z()) < (p1.z() - p2.z()) * (p1.y() - p0.y());
        if (axis === Triangle.ProjectionAxis.Y)
            return (p1.x() - p2.x()) * (p1.z() - p0.z()) < (p1.z() - p2.z()) * (p1.x() - p0.x());
        return (p1.x() - p2.x()) * (p1.y() - p0.y()) < (p1.y() - p2.y()) * (p1.x() - p0.x());
    }

    // Apply the optional vertex swap before testing projected containment.
    public static containsProjected(
        p0: Vector3Dd,
        p1: Vector3Dd,
        p2: Vector3Dd,
        point: Vector3Dd,
        axis: Triangle.ProjectionAxis,
    ): boolean {
        let a0, a1, a2, b0, b1, b2, s, t: number;
        if (axis === Triangle.ProjectionAxis.X) {
            a0 = p0.y();
            a1 = p1.y();
            a2 = p2.y();
            b0 = p0.z();
            b1 = p1.z();
            b2 = p2.z();
            s = point.y();
            t = point.z();
        } else if (axis === Triangle.ProjectionAxis.Y) {
            a0 = p0.x();
            a1 = p1.x();
            a2 = p2.x();
            b0 = p0.z();
            b1 = p1.z();
            b2 = p2.z();
            s = point.x();
            t = point.z();
        } else {
            a0 = p0.x();
            a1 = p1.x();
            a2 = p2.x();
            b0 = p0.y();
            b1 = p1.y();
            b2 = p2.y();
            s = point.x();
            t = point.y();
        }
        if ((a1 - s) * (b1 - b0) < (b1 - t) * (a1 - a0)) return false;
        if ((a2 - s) * (b2 - b1) < (b2 - t) * (a2 - a1)) return false;
        if ((a0 - s) * (b0 - b2) < (b0 - t) * (a0 - a2)) return false;
        return true;
    }

    // Shared allocation-free Möller–Trumbore query; returns affine t and the
    // barycentric coordinates corresponding to v1 and v2.
    public static intersectRay(
        ray: Ray,
        v0: Vector3Dd,
        v1: Vector3Dd,
        v2: Vector3Dd,
        t: number[],
        u: number[],
        v: number[],
    ): boolean {
        return (
            Triangle.intersectRayCore(
                ray,
                v0,
                v1,
                v2,
                VSDK.EPSILON,
                Double.POSITIVE_INFINITY,
                VSDK.EPSILON,
                false,
                false,
                t,
                u,
                v,
            ) === RayFirstHitStatus.Hit
        );
    }

    private static intersectRayCore(
        ray: Ray,
        v0: Vector3Dd,
        v1: Vector3Dd,
        v2: Vector3Dd,
        minT: number,
        maxT: number,
        determinantTolerance: number,
        includeMin: boolean,
        checkArithmetic: boolean,
        t: number[],
        u: number[],
        v: number[],
    ): RayFirstHitStatus {
        const e1 = v1.subtract(v0);
        const e2 = v2.subtract(v0);
        const h = ray.getDirection().crossProduct(e2);
        const determinant = e1.dotProduct(h);
        if (
            checkArithmetic &&
            (!Double.isFinite(e1.x()) ||
                !Double.isFinite(e1.y()) ||
                !Double.isFinite(e1.z()) ||
                !Double.isFinite(e2.x()) ||
                !Double.isFinite(e2.y()) ||
                !Double.isFinite(e2.z()) ||
                !Double.isFinite(determinant))
        )
            return RayFirstHitStatus.Indeterminate;
        let geometricNormal = new Vector3Dd();
        if (checkArithmetic) {
            geometricNormal = e1.crossProduct(e2);
            const squared = geometricNormal.dotProduct(geometricNormal);
            if (!Double.isFinite(squared)) return RayFirstHitStatus.Indeterminate;
            if (squared === 0)
                return geometricNormal.x() === 0 && geometricNormal.y() === 0 && geometricNormal.z() === 0
                    ? RayFirstHitStatus.Miss
                    : RayFirstHitStatus.Indeterminate;
        }
        if (checkArithmetic && determinant === 0) {
            const normal = geometricNormal;
            const direction = ray.getDirection();
            const normalDotDirection = normal.dotProduct(direction);
            if (
                !Double.isFinite(normalDotDirection) ||
                normalDotDirection !== 0 ||
                (normal.x() !== 0 && direction.x() !== 0 && normal.x() * direction.x() === 0) ||
                (normal.y() !== 0 && direction.y() !== 0 && normal.y() * direction.y() === 0) ||
                (normal.z() !== 0 && direction.z() !== 0 && normal.z() * direction.z() === 0)
            )
                return RayFirstHitStatus.Indeterminate;
            const side = ray.getOrigin().subtract(v0).dotProduct(normal);
            if (!Double.isFinite(side)) return RayFirstHitStatus.Indeterminate;
            return side === 0 ? RayFirstHitStatus.Indeterminate : RayFirstHitStatus.Miss;
        }
        if (JavaMath.abs(determinant) < determinantTolerance) return RayFirstHitStatus.Miss;
        const inverse = 1.0 / determinant;
        const offset = ray.getOrigin().subtract(v0);
        u[0] = inverse * offset.dotProduct(h);
        if (checkArithmetic && !Double.isFinite(u[0])) return RayFirstHitStatus.Indeterminate;
        if (u[0] < 0.0 || u[0] > 1.0) return RayFirstHitStatus.Miss;
        const q = offset.crossProduct(e1);
        v[0] = inverse * ray.getDirection().dotProduct(q);
        if (checkArithmetic && !Double.isFinite(v[0])) return RayFirstHitStatus.Indeterminate;
        if (v[0] < 0.0 || u[0] + v[0] > 1.0) return RayFirstHitStatus.Miss;
        t[0] = inverse * e2.dotProduct(q);
        if (checkArithmetic && !Double.isFinite(t[0])) return RayFirstHitStatus.Indeterminate;
        return (includeMin ? t[0] >= minT : t[0] > minT) && t[0] <= maxT
            ? RayFirstHitStatus.Hit
            : RayFirstHitStatus.Miss;
    }

    public static containmentTest(a: Vector3Dd, b: Vector3Dd, c: Vector3Dd, p: Vector3Dd, t: number): number {
        const ab = b.subtract(a),
            ac = c.subtract(a),
            ap = p.subtract(a),
            d1 = ab.dotProduct(ap),
            d2 = ac.dotProduct(ap),
            denom = ab.dotProduct(ab) * ac.dotProduct(ac) - ab.dotProduct(ac) ** 2;
        if (Math.abs(denom) < VSDK.EPSILON) return Containment.OUTSIDE;
        const u = (d1 * ac.dotProduct(ac) - d2 * ab.dotProduct(ac)) / denom,
            v = (d2 * ab.dotProduct(ab) - d1 * ab.dotProduct(ac)) / denom,
            q = a.add(ab.multiply(Math.max(0, Math.min(1, u)))).add(ac.multiply(Math.max(0, Math.min(1, v))));
        return q.subtract(p).length() <= t ? Containment.LIMIT : Containment.OUTSIDE;
    }
    public static minMax(a: Vector3Dd, b: Vector3Dd, c: Vector3Dd, m: Float64Array | number[]): void {
        m[0] = Math.min(a.x(), b.x(), c.x());
        m[1] = Math.min(a.y(), b.y(), c.y());
        m[2] = Math.min(a.z(), b.z(), c.z());
        m[3] = Math.max(a.x(), b.x(), c.x());
        m[4] = Math.max(a.y(), b.y(), c.y());
        m[5] = Math.max(a.z(), b.z(), c.z());
    }
    public override toString() {
        return `f < ${this.point0}, ${this.point1}, ${this.point2} >`;
    }
}

export namespace Triangle {
    export enum ProjectionAxis {
        X = 0,
        Y = 1,
        Z = 2,
    }
}
