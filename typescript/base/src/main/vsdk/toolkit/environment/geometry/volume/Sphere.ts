import { RayCrossing } from "../element/RayCrossing.js";
import { RayCrossingKind } from "../element/RayCrossingKind.js";
import { RayCrossingSupport } from "../element/RayCrossingSupport.js";
import { RayCrossingQuery } from "../element/RayCrossingQuery.js";
import { RayFirstHitStatus } from "../element/RayFirstHitStatus.js";
import { RaySolidQueryStatus } from "../element/RaySolidQueryStatus.js";
import type { RaySolidClassification } from "../element/RaySolidClassification.js";
import { RaySolidBoundary } from "../element/RaySolidBoundary.js";
import { RayBoundary } from "../element/RayBoundary.js";
import type { GeometryBounds } from "../element/GeometryBounds.js";
import { GeometryContainment } from "../element/GeometryContainment.js";
import type { Geometry } from "../Geometry.js";
import { FloatingInterval } from "../../../common/numerics/FloatingInterval.js";
import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
import { IllegalStateException } from "../../../../../java/lang/IllegalStateException.js";
import type { RayCrossingSink } from "../element/RayCrossingSink.js";
//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { PolyhedralBoundedSolidEulerOperators } from "./polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.js";
import { RaytraceStatistics } from "../../../common/statistics/RaytraceStatistics.js";
import { VSDK } from "../../../common/VSDK.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import type { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import { PolyhedralBoundedSolid } from "./polyhedralBoundedSolid/PolyhedralBoundedSolid.js";
import { Solid } from "./Solid.js";

export class Sphere extends Solid {
    // Returns zero, one or two roots in increasing order without allocation.
    public static intersectionRoots(
        p: Vector3Dd,
        d: Vector3Dd,
        radius: number,
        query: RayCrossingQuery,
        nearT: number[],
        farT: number[],
    ): number {
        const lengthSquared = d.dotProduct(d);
        if (!(lengthSquared > 0.0) || query.minT > query.maxT) return 0;
        const directionLength = Math.sqrt(lengthSquared);
        const unitDirection = d.multiply(1.0 / directionLength);
        const radiusSquared = radius * radius;
        const ocSquared = p.dotProduct(p);
        const inside = ocSquared < radiusSquared;
        const closest = -p.dotProduct(unitDirection);
        if (!inside && closest < query.minimumExteriorApproach) return 0;
        const chordSquared = radiusSquared - ocSquared + closest * closest;
        if (chordSquared < query.minimumChordSquared) return 0;
        const halfChord = Math.sqrt(chordSquared);
        farT[0] = (closest + halfChord) / directionLength;
        nearT[0] = (closest - halfChord) / directionLength;
        const nearValid = nearT[0] >= query.minT && nearT[0] <= query.maxT;
        const farValid = farT[0] >= query.minT && farT[0] <= query.maxT;
        if (!nearValid && !farValid) return 0;
        if (!nearValid) nearT[0] = farT[0];
        if (!farValid) farT[0] = nearT[0];
        return nearT[0] === farT[0] ? 1 : 2;
    }

    public override copyGeometry(): Geometry | null {
        return new Sphere(this.radius);
    }

    public override emitRayCrossings(ray: Ray, query: RayCrossingQuery, sink: RayCrossingSink): RayCrossingSupport {
        const failure = [RayFirstHitStatus.Unsupported];
        if (!this.validateSurfaceQuery(ray, query, failure)) {
            return failure[0] === RayFirstHitStatus.InvalidInput
                ? RayCrossingSupport.InvalidInput
                : RayCrossingSupport.Indeterminate;
        }
        const nearT = [0],
            farT = [0];
        const count = Sphere.intersectionRoots(ray.getOrigin(), ray.getDirection(), this.radius, query, nearT, farT);
        const emit = (t: number): void => {
            const point = ray.getOrigin().add(ray.getDirection().multiply(t));
            const orientation = point.dotProduct(ray.getDirection());
            const kind =
                orientation < 0
                    ? RayCrossingKind.Entering
                    : orientation > 0
                      ? RayCrossingKind.Leaving
                      : RayCrossingKind.Tangent;
            sink.crossing(new RayCrossing(t, this, kind));
        };
        if (count > 0) emit(nearT[0]!);
        if (count > 1) emit(farT[0]!);
        return RayCrossingSupport.Supported;
    }

    public override classifyRay(
        ray: Ray,
        query: RayCrossingQuery,
        result: RaySolidClassification,
    ): RaySolidQueryStatus {
        result.clear();
        const origin = ray.getOrigin();
        const direction = ray.getDirection();
        const infinity = Double.POSITIVE_INFINITY;
        // Surface compatibility cutoffs may discard genuine boundaries and cannot
        // define a solid classification. Applications apply those to surface queries.
        if (
            Double.isNaN(query.minT) ||
            Double.isNaN(query.maxT) ||
            query.minT >= query.maxT ||
            query.minimumChordSquared !== 0.0 ||
            query.minimumExteriorApproach !== -infinity ||
            !Double.isFinite(this.radius) ||
            this.radius <= 0.0 ||
            !Double.isFinite(origin.x()) ||
            !Double.isFinite(origin.y()) ||
            !Double.isFinite(origin.z()) ||
            !Double.isFinite(direction.x()) ||
            !Double.isFinite(direction.y()) ||
            !Double.isFinite(direction.z()) ||
            (direction.x() === 0 && direction.y() === 0 && direction.z() === 0)
        ) {
            return RaySolidQueryStatus.InvalidInput;
        }
        const lengthSquared = direction.dotProduct(direction);
        if (
            !Double.isFinite(lengthSquared) ||
            lengthSquared === 0 ||
            !Double.isFinite(origin.dotProduct(origin)) ||
            !Double.isFinite(this.radius * this.radius) ||
            this.radius * this.radius === 0.0
        ) {
            return RaySolidQueryStatus.Indeterminate;
        }
        // Obtain the complete analytic interval before clipping: an empty set of
        // surface events in the requested range can still mean entirely inside.
        const nearT = [0],
            farT = [0];
        const count = Sphere.intersectionRoots(
            origin,
            direction,
            this.radius,
            new RayCrossingQuery(-infinity, infinity),
            nearT,
            farT,
        );
        if (!Double.isFinite(nearT[0]!) || !Double.isFinite(farT[0]!)) {
            return RaySolidQueryStatus.Indeterminate;
        }
        if (count < 2) {
            if (!result.insideSegments.add(false)) {
                result.clear();
                return RaySolidQueryStatus.Indeterminate;
            }
            return RaySolidQueryStatus.Supported; // Miss or point tangency (IN/OUT).
        }
        if (!result.insideSegments.add(query.minT > nearT[0]! && query.minT <= farT[0]!)) {
            result.clear();
            return RaySolidQueryStatus.Indeterminate;
        }
        if (nearT[0]! >= query.minT && nearT[0]! <= query.maxT) {
            if (!result.boundaries.add(new RaySolidBoundary(nearT[0]!, this)) || !result.insideSegments.add(true)) {
                result.clear();
                return RaySolidQueryStatus.Indeterminate;
            }
        }
        if (farT[0]! >= query.minT && farT[0]! <= query.maxT) {
            if (!result.boundaries.add(new RaySolidBoundary(farT[0]!, this)) || !result.insideSegments.add(false)) {
                result.clear();
                return RaySolidQueryStatus.Indeterminate;
            }
        }
        return RaySolidQueryStatus.Supported;
    }

    public override getBounds(result: GeometryBounds): void {
        if (!Double.isFinite(this.radius) || this.radius <= 0) {
            result.setIndeterminate();
            return;
        }
        result.setFinite(
            new Vector3Dd(-this.radius, -this.radius, -this.radius),
            new Vector3Dd(this.radius, this.radius, this.radius),
        );
    }

    private validateSurfaceQuery(ray: Ray, query: RayCrossingQuery, failure: RayFirstHitStatus[]): boolean {
        const p = ray.getOrigin();
        const d = ray.getDirection();
        if (
            Double.isNaN(query.minT) ||
            Double.isNaN(query.maxT) ||
            query.minT > query.maxT ||
            !Double.isFinite(query.minimumChordSquared) ||
            query.minimumChordSquared < 0 ||
            Double.isNaN(query.minimumExteriorApproach) ||
            !Double.isFinite(this.radius) ||
            this.radius <= 0 ||
            !Double.isFinite(p.x()) ||
            !Double.isFinite(p.y()) ||
            !Double.isFinite(p.z()) ||
            !Double.isFinite(d.x()) ||
            !Double.isFinite(d.y()) ||
            !Double.isFinite(d.z()) ||
            (d.x() === 0 && d.y() === 0 && d.z() === 0)
        ) {
            failure[0] = RayFirstHitStatus.InvalidInput;
            return false;
        }
        const lengthSquared = d.dotProduct(d);
        const originSquared = p.dotProduct(p);
        const rSquared = this.radius * this.radius;
        if (
            !Double.isFinite(lengthSquared) ||
            lengthSquared === 0 ||
            !Double.isFinite(originSquared) ||
            !Double.isFinite(rSquared) ||
            rSquared === 0
        ) {
            failure[0] = RayFirstHitStatus.Indeterminate;
            return false;
        }
        return true;
    }

    public override queryFirstHit(
        ray: Ray,
        query: RayCrossingQuery,
        result: RayHit | null,
        boundary: RayBoundary | null = null,
    ): RayFirstHitStatus {
        if (result) result.clear();
        if (boundary) boundary.assign(new RayBoundary());
        const p = ray.getOrigin();
        const d = ray.getDirection();
        const failure = [RayFirstHitStatus.Unsupported];
        if (!this.validateSurfaceQuery(ray, query, failure)) return failure[0]!;
        const nearT = [0],
            farT = [0];
        if (Sphere.intersectionRoots(p, d, this.radius, query, nearT, farT) === 0) return RayFirstHitStatus.Miss;
        if (!Double.isFinite(nearT[0]!)) return RayFirstHitStatus.Indeterminate;
        if (boundary) boundary.assign(new RayBoundary(nearT[0]!, this));
        if (result) {
            result.setHitDistance(nearT[0]!);
            if (result.shouldStoreRay() || result.needsAnySurfaceData()) {
                const hitRay = ray.withT(nearT[0]!);
                result.setRay(hitRay);
                if (result.needsAnySurfaceData()) {
                    this.fillSurfaceDetail(hitRay, nearT[0]!, result);
                    result.setRay(hitRay);
                }
            }
        }
        return RayFirstHitStatus.Hit;
    }

    private radius: number;
    private radiusSquared: number;

    private static readonly DEFAULT_PARALLELS = 8;
    private static readonly DEFAULT_MERIDIANS = 16;
    private static readonly MIN_PARALLELS = 2;
    private static readonly MIN_MERIDIANS = 3;

    public constructor(r: number) {
        super();
        this.radius = r;
        this.radiusSquared = this.radius * this.radius;
        this.getControlSpecifications().push("double;radius;(0, INFINITE)");
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.

    Java overload `doIntersectionFirstHit(Ray inout_rayo)`.
    @param inout_rayo
    @return true if given ray intersects current Sphere
    */
    public doIntersectionFirstHit(inout_rayo: Ray): Ray | null;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit): boolean;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit | null): boolean;
    // Explicit inclusive interval; the legacy overload queries [0,+infinity).
    public doIntersectionFirstHit(inRay: Ray, query: RayCrossingQuery, outHit: RayHit | null): boolean;
    public override doIntersectionFirstHit(
        inRay: Ray,
        second?: RayHit | RayCrossingQuery | null,
        third?: RayHit | null,
    ): Ray | null | boolean {
        if (second === undefined) {
            const nearT = [0],
                farT = [0];
            if (
                Sphere.intersectionRoots(
                    inRay.getOrigin(),
                    inRay.getDirection(),
                    this.radius,
                    new RayCrossingQuery(0, Double.POSITIVE_INFINITY),
                    nearT,
                    farT,
                ) === 0
            ) {
                return null;
            }
            return inRay.withT(nearT[0]!);
        }
        if (!(second instanceof RayCrossingQuery)) {
            return this.doIntersectionFirstHit(inRay, new RayCrossingQuery(0, Double.POSITIVE_INFINITY), second);
        }
        const query = second;
        const outHit = third ?? null;
        const t = [0],
            farT = [0];
        if (Sphere.intersectionRoots(inRay.getOrigin(), inRay.getDirection(), this.radius, query, t, farT) === 0) {
            return false;
        }

        if (outHit !== null) {
            if (outHit.shouldStoreRay() || outHit.needsAnySurfaceData()) {
                const hitRay = inRay.withT(t[0]!);
                outHit.setRay(hitRay);
                if (outHit.needsAnySurfaceData()) {
                    this.doExtraInformation(hitRay, t[0]!, outHit);
                    outHit.setRay(hitRay);
                }
            } else {
                outHit.setHitDistance(t[0]!);
            }
        }
        return true;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    @param inT
    */
    public override doExtraInformation(inRay: Ray, inT: number, outData: RayHit): void {
        this.fillSurfaceDetail(inRay, inT, outData);
    }

    private fillSurfaceDetail(inRay: Ray, inT: number, outData: RayHit | null): void {
        if (outData === null) {
            return;
        }

        outData.beginDetailEvaluation();
        RaytraceStatistics.recordGeometryDetailComputation();
        const needsNormalVector = outData.needsNormal() || outData.needsTextureCoordinates() || outData.needsTangent();
        if (!outData.needsPoint() && !needsNormalVector) {
            return;
        }

        const point = new Vector3Dd(
            inRay.getOrigin().x() + inT * inRay.getDirection().x(),
            inRay.getOrigin().y() + inT * inRay.getDirection().y(),
            inRay.getOrigin().z() + inT * inRay.getDirection().z(),
        );
        if (outData.needsPoint()) {
            outData.point = point;
            if (Double.isFinite(point.x()) && Double.isFinite(point.y()) && Double.isFinite(point.z()))
                outData.markDetailAvailable(RayHit.DETAIL_POINT);
        }

        let normal: Vector3Dd | null = null;
        if (needsNormalVector) {
            const pointSquared = point.dotProduct(point);
            // Vector3Dd::normalized intentionally treats lengths below VSDK::EPSILON
            // as zero; a small but valid sphere still needs a unit surface normal.
            if (Double.isFinite(pointSquared) && pointSquared > 0 && pointSquared < VSDK.EPSILON)
                normal = point.multiply(1.0 / JavaMath.sqrt(pointSquared));
            else normal = point.normalized();
            if (outData.needsNormal()) {
                outData.normal = normal;
                const normalSquared = normal.dotProduct(normal);
                if (
                    Double.isFinite(normalSquared) &&
                    normalSquared > 0 &&
                    Double.isFinite(normal.x()) &&
                    Double.isFinite(normal.y()) &&
                    Double.isFinite(normal.z())
                )
                    outData.markDetailAvailable(RayHit.DETAIL_NORMAL);
            }
        }

        if (!outData.needsTextureCoordinates() && !outData.needsTangent()) {
            return;
        }

        let theta: number;
        const phi = Math.acos(normal!.z());

        if (normal!.x() > VSDK.EPSILON) {
            theta = Math.atan(normal!.y() / normal!.x()) + (3 * Math.PI) / 2;
        } else if (normal!.x() < VSDK.EPSILON) {
            theta = Math.atan(normal!.y() / normal!.x()) + (3 * Math.PI) / 2;
            theta += Math.PI;
            if (theta > 2 * Math.PI) {
                theta -= 2 * Math.PI;
            }
        } else {
            theta = 0.0;
        }

        if (outData.needsTextureCoordinates()) {
            outData.u = (theta + Math.PI / 2) / (2 * Math.PI);
            outData.v = 1 - phi / Math.PI;
            if (Double.isFinite(outData.u) && Double.isFinite(outData.v)) outData.markDetailAvailable(RayHit.DETAIL_UV);
        }
        if (outData.needsTangent()) {
            outData.tangent = new Vector3Dd(Math.sin(theta - Math.PI / 2), -Math.cos(theta - Math.PI / 2), 0);
            if (Double.isFinite(outData.tangent.x()) && Double.isFinite(outData.tangent.y()))
                outData.markDetailAvailable(RayHit.DETAIL_TANGENT);
        }
    }

    public override classifyBoxCertified(center: Vector3Dd, halfWidths: Vector3Dd): GeometryContainment {
        if (!Double.isFinite(this.radius) || this.radius <= 0) return GeometryContainment.InvalidInput;
        if (!FloatingInterval.isSupportedEnvironment()) return GeometryContainment.Unsupported;
        const c = [center.x(), center.y(), center.z()];
        const h = [halfWidths.x(), halfWidths.y(), halfWidths.z()];
        // Squared distances from the centre to the farthest and nearest box points.
        let farthest = FloatingInterval.exact(0),
            nearest = FloatingInterval.exact(0);
        for (let i = 0; i < 3; ++i) {
            const range = FloatingInterval.around(c[i]!, h[i]!);
            if (!range.isValid()) return GeometryContainment.InvalidInput;
            const extreme = JavaMath.max(JavaMath.abs(range.getLower()), JavaMath.abs(range.getUpper()));
            farthest = farthest.add(FloatingInterval.exact(extreme).square());
            if (range.getLower() > 0) nearest = nearest.add(FloatingInterval.exact(range.getLower()).square());
            else if (range.getUpper() < 0) nearest = nearest.add(FloatingInterval.exact(range.getUpper()).square());
        }
        const squaredRadius = FloatingInterval.exact(this.radius).square();
        if (!farthest.isValid() || !nearest.isValid() || !squaredRadius.isValid())
            return GeometryContainment.Indeterminate;
        if (farthest.getUpper() < squaredRadius.getLower()) return GeometryContainment.Inside;
        if (nearest.getLower() > squaredRadius.getUpper()) return GeometryContainment.Outside;
        return GeometryContainment.Indeterminate;
    }

    public override classifyPointCertified(p: Vector3Dd, distanceTolerance: number): GeometryContainment {
        if (
            !Double.isFinite(this.radius) ||
            this.radius <= 0 ||
            !Double.isFinite(distanceTolerance) ||
            distanceTolerance < 0 ||
            !Double.isFinite(p.x()) ||
            !Double.isFinite(p.y()) ||
            !Double.isFinite(p.z())
        )
            return GeometryContainment.InvalidInput;
        if (!FloatingInterval.isSupportedEnvironment()) return GeometryContainment.Unsupported;
        let squared = FloatingInterval.exact(0);
        squared = squared.add(FloatingInterval.exact(p.x()).square());
        squared = squared.add(FloatingInterval.exact(p.y()).square());
        squared = squared.add(FloatingInterval.exact(p.z()).square());
        const signedDistance = squared.squareRoot().subtract(FloatingInterval.exact(this.radius));
        if (!signedDistance.isValid()) return GeometryContainment.Indeterminate;
        if (signedDistance.getUpper() < -distanceTolerance) return GeometryContainment.Inside;
        if (signedDistance.getLower() > distanceTolerance) return GeometryContainment.Outside;
        if (signedDistance.getLower() >= -distanceTolerance && signedDistance.getUpper() <= distanceTolerance)
            return GeometryContainment.Boundary;
        return GeometryContainment.Indeterminate;
    }

    public override classifyPoint(p: Vector3Dd, distanceTolerance: number): GeometryContainment {
        if (
            !Double.isFinite(this.radius) ||
            this.radius <= 0 ||
            !Double.isFinite(distanceTolerance) ||
            distanceTolerance < 0 ||
            !Double.isFinite(p.x()) ||
            !Double.isFinite(p.y()) ||
            !Double.isFinite(p.z())
        )
            return GeometryContainment.InvalidInput;
        if (distanceTolerance > 0) return this.classifyPointCertified(p, distanceTolerance);
        const squared = p.dotProduct(p);
        const radiusSquaredValue = this.radius * this.radius;
        if (
            !Double.isFinite(squared) ||
            !Double.isFinite(this.radius + distanceTolerance) ||
            !Double.isFinite(radiusSquaredValue) ||
            radiusSquaredValue === 0 ||
            (squared === 0 && (p.x() !== 0 || p.y() !== 0 || p.z() !== 0))
        )
            return GeometryContainment.Indeterminate;
        const distance = p.length();
        if (distance < this.radius - distanceTolerance) return GeometryContainment.Inside;
        if (distance > this.radius + distanceTolerance) return GeometryContainment.Outside;
        return GeometryContainment.Boundary;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest.
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    public override doContainmentTest(p: Vector3Dd, distanceTolerance: number): number {
        const result = this.classifyPoint(p, distanceTolerance);
        if (result === GeometryContainment.Inside) return Sphere.INSIDE;
        if (result === GeometryContainment.Boundary) return Sphere.LIMIT;
        if (result === GeometryContainment.Outside) return Sphere.OUTSIDE;
        // The legacy integer contract has no representation for uncertainty.
        // Do not silently convert an indeterminate or failed query to outside.
        throw new IllegalStateException("Sphere containment cannot be represented by the legacy integer result");
    }

    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    public override getMinMax(): Float64Array {
        const minmax = new Float64Array(6);
        for (let i = 0; i < 3; i++) {
            minmax[i] = -this.radius;
        }
        for (let i = 3; i < 6; i++) {
            minmax[i] = this.radius;
        }
        return minmax;
    }

    public getRadius(): number {
        return this.radius;
    }

    // C++ returns a borrowed reference to the intrinsic radius; numbers are values here.
    public getRadiusReference(): number {
        return this.radius;
    }
    // Signed squared implicit function: negative inside, positive outside.
    public implicitValue(point: Vector3Dd): number {
        return point.dotProduct(point) - this.radiusSquared;
    }
    // Fast normal for a known nonzero surface point in local coordinates.
    public surfaceNormalAtNonzeroPoint(point: Vector3Dd): Vector3Dd {
        return point.normalizedFast();
    }

    public getRadiusSquared(): number {
        return this.radiusSquared;
    }

    public setRadius(value: number): void {
        this.radius = value;
        this.radiusSquared = value * value;
    }

    private static spherePosition(theta: number, t: number, r: number): Vector3Dd {
        const phi = (t - 0.5) * Math.PI;
        return new Vector3Dd(
            Math.cos(phi) * Math.cos(theta) * r,
            Math.cos(phi) * Math.sin(theta) * r,
            Math.sin(phi) * r,
        );
    }

    /**
    Java overloads `exportToPolyhedralBoundedSolid()` and
    `exportToPolyhedralBoundedSolid(int meridians, int parallels)`, with the
    C++ `withQuadrilaterals` parameter (default false).

    Boundary representation of the sphere, centered at the origin with its
    poles on the Z axis: `parallels` bands of equal latitude step between the
    poles, each one cut into `meridians` sectors starting at +X. The bands
    touching the poles are triangle fans; the faces of the other bands are
    the planar quadrilaterals between two parallels and two meridians (two
    points of each parallel, at the same longitudes).
    @param meridians number of sectors around the Z axis (at least 3)
    @param parallels number of bands from pole to pole (at least 2)
    @param withQuadrilaterals false (default) splits each quadrilateral by a
    diagonal into two triangles, so every face is a triangle; true keeps the
    quadrilaterals as faces
    @return a new boundary representation
    */
    public override exportToPolyhedralBoundedSolid(): PolyhedralBoundedSolid;
    public override exportToPolyhedralBoundedSolid(
        meridians: number,
        parallels: number,
        withQuadrilaterals?: boolean,
    ): PolyhedralBoundedSolid;
    public override exportToPolyhedralBoundedSolid(
        meridians?: number,
        parallels?: number,
        withQuadrilaterals = false,
    ): PolyhedralBoundedSolid {
        if (meridians === undefined || parallels === undefined) {
            return this.buildPolyhedralBoundedSolid(Sphere.DEFAULT_MERIDIANS, Sphere.DEFAULT_PARALLELS, false);
        }
        const normalizedMeridians = Math.max(Sphere.MIN_MERIDIANS, meridians);
        const normalizedParallels = Math.max(Sphere.MIN_PARALLELS, parallels);

        if (
            !withQuadrilaterals &&
            normalizedMeridians === Sphere.DEFAULT_MERIDIANS &&
            normalizedParallels === Sphere.DEFAULT_PARALLELS
        ) {
            return this.exportToPolyhedralBoundedSolid();
        }

        return this.buildPolyhedralBoundedSolid(normalizedMeridians, normalizedParallels, withQuadrilaterals);
    }

    /**
    Given current sphere, this method generates a "polyhedral ball"
    approximation.
    Note that this method follows a similar strategy to the one proposed on
    function "ball", from program [MANT1988].12.6, but it is expressed entirely
    on "low level" operators, and doesn't rely on the previous availability of
    generalized rotational sweep operations.

    <p><b>Planarity guarantee for the default tessellation</b> (meridians=16,
    parallels=8): each latitude band produces quadrilaterals whose four
    vertices lie on the surface of the sphere at a constant latitude
    {@code phi} and two adjacent longitudes {@code lambda} and
    {@code lambda + 2*PI/meridians}.  All four vertices share the same
    {@code z = r*sin(phi)} coordinate, so the quad is planar in the
    horizontal plane at height {@code z}.  This was verified analytically
    in stage-1 analysis (2026-05-12) and holds for any (meridians, parallels)
    combination because the vertex positions are computed with
    {@code cos}/{@code sin} only in the latitude direction.</p>
    */
    private buildPolyhedralBoundedSolid(
        nmeridians: number,
        nparalels: number,
        withQuadrilaterals: boolean,
    ): PolyhedralBoundedSolid {
        let theta: number;
        let phi: number;
        const dtheta = (2 * Math.PI) / nmeridians;
        const dphi = 1.0 / nparalels;
        let i: number;
        let base2: number;
        let base1: number;
        let pos: Vector3Dd;

        //- Build triangles for lower cap ---------------------------------
        const solid = new PolyhedralBoundedSolid();
        pos = new Vector3Dd(0, 0, -this.radius);
        PolyhedralBoundedSolidEulerOperators.mvfs(solid, pos, 1, 1);

        pos = new Vector3Dd();
        pos = Sphere.spherePosition(dtheta, dphi, this.radius);
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 1, 3, pos);
        pos = new Vector3Dd();
        pos = Sphere.spherePosition(0, dphi, this.radius);
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 3, 2, pos);

        PolyhedralBoundedSolidEulerOperators.mef(solid, 1, 1, 1, 3, 2, 3, 2);

        for (i = 2; i < nmeridians; i++) {
            theta = dtheta * i;
            pos = new Vector3Dd();
            pos = Sphere.spherePosition(theta, dphi, this.radius);
            PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 1, i + 1 + 1, pos);
            // Next face is <(1), (i+1), (i+0)>
            PolyhedralBoundedSolidEulerOperators.mef(
                solid,
                1 /* seed face, always face 1 */,
                1 /* seed face, always face 1 */,
                i + 0 + 1 /* start of half edge 1 */,
                1 /* end of half edge 1 */,
                i + 1 + 1 /* start of half edge 2 */,
                1 /* end of half edge 2 */,
                i + 1 /* new face id */,
            );
        }
        // Next face is <(1), (2), (i+1)>
        PolyhedralBoundedSolidEulerOperators.mef(
            solid,
            1 /* seed face, always face 1 */,
            1 /* seed face, always face 1 */,
            i + 1 /* start of half edge 1 */,
            1 /* end of half edge 1 */,
            2 /* start of half edge 2 */,
            3 /* end of half edge 2 */,
            i + 1 /* new face id */,
        );
        base2 = i + 2;
        base1 = 2;

        //- Build triangulated side strips for sphere body ----------------
        // Each spherical quad is split into two triangles immediately after
        // creation via smef, adding a diagonal from TL corner to BR corner.
        // Face IDs are tracked with a running counter to avoid conflicts with
        // the doubled face count.
        let p: number;
        let nextFaceId = nmeridians + 2;
        for (p = 0; p < nparalels - 2; p++) {
            phi = (p + 2) / nparalels;
            for (i = 0; i < nmeridians; i++) {
                theta = dtheta * i;
                pos = new Vector3Dd();
                pos = Sphere.spherePosition(theta, phi, this.radius);
                PolyhedralBoundedSolidEulerOperators.smev(solid, 1, i + base1, i + base2, pos);
                if (i > 0) {
                    const quadFaceId = nextFaceId++;
                    PolyhedralBoundedSolidEulerOperators.mef(
                        solid,
                        1 /* seed face, always face 1 */,
                        1 /* seed face, always face 1 */,
                        i - 1 + base2 /* start of half edge 1: TL */,
                        i - 1 + base1 /* end of half edge 1: BL */,
                        i + base2 /* start of half edge 2: TR */,
                        i + base1 /* end of half edge 2: BR */,
                        quadFaceId,
                    );
                    if (!withQuadrilaterals) {
                        const diagFaceId = nextFaceId++;
                        // Split quad {TR,TL,BL,BR} into two triangles via TL→BR diagonal
                        PolyhedralBoundedSolidEulerOperators.smef(
                            solid,
                            quadFaceId,
                            i - 1 + base2 /* TL vertex */,
                            i + base1 /* BR vertex */,
                            diagFaceId,
                        );
                    }
                }
            }
            {
                const quadFaceId = nextFaceId++;
                // Wrap-around quad: closes the ring from the last meridian back to the first
                PolyhedralBoundedSolidEulerOperators.mef(
                    solid,
                    1 /* seed face, always face 1 */,
                    1 /* seed face, always face 1 */,
                    i + base2 - 1 /* start of half edge 1: TL-last */,
                    base1 + i - 1 /* end of half edge 1: BL-last */,
                    base2 /* start of half edge 2 */,
                    base2 + 1 /* end of half edge 2 */,
                    quadFaceId,
                );
                if (!withQuadrilaterals) {
                    const diagFaceId = nextFaceId++;
                    // Split wrap-around quad via TL-last → BL-0 diagonal
                    PolyhedralBoundedSolidEulerOperators.smef(
                        solid,
                        quadFaceId,
                        i - 1 + base2 /* TL-last vertex */,
                        base1 /* BL-0 vertex (first old-parallel vertex) */,
                        diagFaceId,
                    );
                }
            }
            base1 = base2;
            base2 += nmeridians;
        }

        //- Build triangles for upper cap --------------------------------
        pos = new Vector3Dd(0, 0, this.radius);
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, base1, base2, pos);

        for (i = 0; i < nmeridians - 2; i++) {
            PolyhedralBoundedSolidEulerOperators.mef(
                solid,
                1 /* seed face, always face 1 */,
                1 /* seed face, always face 1 */,
                base2 /* start of half edge 1 */,
                base1 + i /* end of half edge 1 */,
                base1 + i + 1 /* start of half edge 2 */,
                base1 + i + 2 /* end of half edge 2 */,
                nextFaceId++,
            );
        }

        PolyhedralBoundedSolidEulerOperators.mef(
            solid,
            1 /* seed face, always face 1 */,
            1 /* seed face, always face 1 */,
            base2 /* start of half edge 1 */,
            base1 + i /* end of half edge 1 */,
            base1 + i + 1 /* start of half edge 2 */,
            base1 /* end of half edge 2 */,
            nextFaceId++,
        );

        //-----------------------------------------------------------------
        return solid;
    }

    /**
    Given a (thetha, phi) spherical coordinate in the surface of current
    Sphere, this method writes on to `p` Vector3Dd the (x, y, z) coordinates
    of the corresponding point on Sphere's surface.
    \todo  check this method for efficiency improvement
    @param p
    @param theta
    @param phi
    */
    public spherePosition(theta: number, phi: number): Vector3Dd {
        return new Vector3Dd(
            Math.cos(phi) * Math.cos(theta) * this.radius,
            -Math.cos(phi) * Math.sin(theta) * this.radius,
            Math.sin(phi) * this.radius,
        );
    }

    /**
    Given a (thetha, phi) spherical coordinate in the surface of current
    Sphere, this method writes on to `n` Vector3Dd the (nx, ny, nz) coordinates
    of the surface normal at corresponding point on Sphere's surface.
    \todo  check this method for efficiency improvement
    @param n
    @param theta
    @param phi
    */
    public sphereNormal(theta: number, phi: number): Vector3Dd {
        return new Vector3Dd(Math.cos(phi) * Math.cos(theta), -Math.cos(phi) * Math.sin(theta), Math.sin(phi));
    }

    /**
    Given a (thetha, phi) spherical coordinate in the surface of current
    Sphere, this method writes on to `n` Vector3Dd the (tx, ty, tz) coordinates
    of the surface tangent at corresponding point on Sphere's surface. Tangents
    are aligned with respect to Sphere's equator.
    \todo  check this method for efficiency improvement
    \todo  check this method for efficiency improvement
    @param t
    @param theta
    @param phi
    */
    public sphereTangent(theta: number, _phi: number): Vector3Dd {
        return new Vector3Dd(Math.sin(theta), Math.cos(theta), 0);
    }

    /**
    Given a (thetha, phi) spherical coordinate in the surface of current
    Sphere, this method writes on to `n` Vector3Dd the (bx, by, bz) coordinates
    of the surface tangent binormal at corresponding point on Sphere's surface.
    Tangents binormals are perpendicular to both normal and tangent.
    \todo  check this method for efficiency improvement
    @param b
    @param theta
    @param phi
    */
    public sphereBinormal(theta: number, phi: number): Vector3Dd {
        return new Vector3Dd(
            -Math.sin(phi) * Math.cos(theta),
            Math.sin(phi) * Math.sin(theta),
            Math.cos(phi) * Math.cos(theta) * Math.cos(theta) + Math.cos(phi) * Math.sin(theta) * Math.sin(theta),
        );
    }
}
