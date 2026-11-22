import { FundamentalEntity } from "../../../common/FundamentalEntity.js";
import { Vector2Dd } from "../../../common/linealAlgebra/Vector2Dd.js";
import { Vector3Df } from "../../../common/linealAlgebra/Vector3Df.js";
import { Logger } from "../../../common/logging/Logger.js";
import { String as JavaString } from "../../../../../java/lang/String.js";

// Single precision rounding: every float operation of the source is rounded
// with it, so that results are the IEEE single precision ones bit for bit
const f = Math.fround;

/**
A planar triangle or quadrilateral in single precision, as used by the
radiosity solvers: the face of a polygonal mesh with its plane, area,
parametrization and uniform (area preserving) sampling.

The face copies its three or four vertex positions when it is built and
never changes afterwards, so a face can be shared by concurrent readers.

Every quantity is computed in single precision, in source operation order and
with IEEE semantics; do not rewrite it with vector helpers or in double
precision. In this port each single precision operation is rounded with
Math.fround, and values promoted to double precision in the source are left
unrounded.

Parametrizations:
- triangle: barycentric, point = p0 + u (p1 - p0) + v (p2 - p0)
- quadrilateral: bi-linear, point = p0 + (u - uv) (p1 - p0) + uv (p2 - p0)
  + (v - uv) (p3 - p0)
The Jacobian of the quadrilateral mapping is J(u, v) = A + B u + C v; for
triangles and parallelograms it is constant and equal to the area, and the
face reports that it has no Jacobian.
*/
export class PlanarFace extends FundamentalEntity {
    public static readonly MAXIMUM_VERTICES = 4;
    public static readonly EPSILON = 1e-6;
    public static readonly EPSILON_FLOAT = f(1e-6);
    private static readonly TOLERANCE = 1e-5;

    // Coordinates kept as plain floats
    private readonly positionX = new Float32Array(PlanarFace.MAXIMUM_VERTICES);
    private readonly positionY = new Float32Array(PlanarFace.MAXIMUM_VERTICES);
    private readonly positionZ = new Float32Array(PlanarFace.MAXIMUM_VERTICES);
    private normalX = 0;
    private normalY = 0;
    private normalZ = 0;
    private midPointX = 0;
    private midPointY = 0;
    private midPointZ = 0;
    private planeConstant = 0;
    private tolerance = 0;
    private area = 0;
    private jacobianA = 0;
    private jacobianB = 0;
    private jacobianC = 0;
    private numberOfVertices = 0;
    private dominantAxis = 0; // 0: x, 1: y, 2: z
    private jacobianPresent = false;
    private status: PlanarFace.BuildStatus = PlanarFace.BuildStatus.UNSUPPORTED_VERTEX_COUNT;

    // Single precision helpers, written out so that the operation order is the
    // one documented by PlanarFace and does not depend on Vector3Df's methods

    private static difference(a: Vector3Df, b: Vector3Df): Vector3Df {
        return new Vector3Df(f(a.x() - b.x()), f(a.y() - b.y()), f(a.z() - b.z()));
    }

    private static cross(a: Vector3Df, b: Vector3Df): Vector3Df {
        return new Vector3Df(
            f(f(a.y() * b.z()) - f(a.z() * b.y())),
            f(f(a.z() * b.x()) - f(a.x() * b.z())),
            f(f(a.x() * b.y()) - f(a.y() * b.x())),
        );
    }

    private static dot(a: Vector3Df, b: Vector3Df): number {
        return f(f(f(a.x() * b.x()) + f(a.y() * b.y())) + f(a.z() * b.z()));
    }

    private static norm(a: Vector3Df): number {
        return f(Math.sqrt(f(f(f(a.x() * a.x()) + f(a.y() * a.y())) + f(a.z() * a.z()))));
    }

    // (1 / s) v, or v when s is within epsilon of zero
    private static inverseScaled(s: number, v: Vector3Df, epsilon: number): Vector3Df {
        const factor = s < -epsilon || s > epsilon ? f(1.0 / s) : 1.0;
        return new Vector3Df(f(factor * v.x()), f(factor * v.y()), f(factor * v.z()));
    }

    private static positionTolerance(p: Vector3Df, epsilon: number): number {
        return f(epsilon * f(f(Math.abs(p.x()) + Math.abs(p.y())) + Math.abs(p.z())));
    }

    private static pointInTriangle(v0: Vector3Df, v1: Vector3Df, v2: Vector3Df, u: number, v: number): Vector3Df {
        return new Vector3Df(
            f(f(v0.x() + f(u * f(v1.x() - v0.x()))) + f(v * f(v2.x() - v0.x()))),
            f(f(v0.y() + f(u * f(v1.y() - v0.y()))) + f(v * f(v2.y() - v0.y()))),
            f(f(v0.z() + f(u * f(v1.z() - v0.z()))) + f(v * f(v2.z() - v0.z()))),
        );
    }

    private static pointInQuadrilateral(
        v0: Vector3Df,
        v1: Vector3Df,
        v2: Vector3Df,
        v3: Vector3Df,
        u: number,
        v: number,
    ): Vector3Df {
        const c = f(u * v);
        const b = f(u - c);
        const d = f(v - c);
        return new Vector3Df(
            f(f(f(v0.x() + f(b * f(v1.x() - v0.x()))) + f(c * f(v2.x() - v0.x()))) + f(d * f(v3.x() - v0.x()))),
            f(f(f(v0.y() + f(b * f(v1.y() - v0.y()))) + f(c * f(v2.y() - v0.y()))) + f(d * f(v3.y() - v0.y()))),
            f(f(f(v0.z() + f(b * f(v1.z() - v0.z()))) + f(c * f(v2.z() - v0.z()))) + f(d * f(v3.z() - v0.z()))),
        );
    }

    public constructor();
    /**
    Builds the face of the given three or four positions, in order. The face is
    invalid (see getStatus) for any other vertex count or when the polygon is
    degenerate (its normal cannot be computed)
    */
    public constructor(inNumberOfVertices: number, inPositions: Vector3Df[]);
    public constructor(inNumberOfVertices?: number, inPositions?: Vector3Df[]) {
        super();
        if (inNumberOfVertices === undefined || inPositions === undefined) {
            return;
        }
        if (inNumberOfVertices !== 3 && inNumberOfVertices !== 4) {
            this.status = PlanarFace.BuildStatus.UNSUPPORTED_VERTEX_COUNT;
            return;
        }
        this.numberOfVertices = inNumberOfVertices;
        for (let i = 0; i < this.numberOfVertices; i++) {
            this.positionX[i] = inPositions[i]!.x();
            this.positionY[i] = inPositions[i]!.y();
            this.positionZ[i] = inPositions[i]!.z();
        }

        if (!this.computeNormal()) {
            this.status = PlanarFace.BuildStatus.DEGENERATE_NORMAL;
            return;
        }
        this.computeAreaAndJacobian();
        this.computeMidPoint();
        this.planeConstant = -PlanarFace.dot(this.getNormal(), this.getMidPoint());
        this.tolerance = this.computeTolerance();
        this.dominantAxis = PlanarFace.dominantCoordinate(this.getNormal());
        this.status = PlanarFace.BuildStatus.VALID;
    }

    public getStatus(): PlanarFace.BuildStatus {
        return this.status;
    }

    public isValid(): boolean {
        return this.status === PlanarFace.BuildStatus.VALID;
    }

    public getNumberOfVertices(): number {
        return this.numberOfVertices;
    }

    public getPosition(i: number): Vector3Df {
        return new Vector3Df(this.positionX[i]!, this.positionY[i]!, this.positionZ[i]!);
    }

    public getNormal(): Vector3Df {
        return new Vector3Df(this.normalX, this.normalY, this.normalZ);
    }

    public getMidPoint(): Vector3Df {
        return new Vector3Df(this.midPointX, this.midPointY, this.midPointZ);
    }

    public getPlaneConstant(): number {
        return this.planeConstant;
    }

    public getTolerance(): number {
        return this.tolerance;
    }

    public getArea(): number {
        return this.area;
    }

    public getDominantAxis(): number {
        return this.dominantAxis;
    }

    public hasJacobian(): boolean {
        return this.jacobianPresent;
    }

    public getJacobianA(): number {
        return this.jacobianA;
    }

    public getJacobianB(): number {
        return this.jacobianB;
    }

    public getJacobianC(): number {
        return this.jacobianC;
    }

    /**
    Newell's method; false for a degenerate polygon
    */
    private computeNormal(): boolean {
        let nx = 0.0;
        let ny = 0.0;
        let nz = 0.0;
        const last = this.numberOfVertices - 1;
        let currentX = f(this.positionX[last]! - this.positionX[0]!);
        let currentY = f(this.positionY[last]! - this.positionY[0]!);
        let currentZ = f(this.positionZ[last]! - this.positionZ[0]!);
        for (let i = 0; i < this.numberOfVertices; i++) {
            const previousX = currentX;
            const previousY = currentY;
            const previousZ = currentZ;
            currentX = f(this.positionX[i]! - this.positionX[0]!);
            currentY = f(this.positionY[i]! - this.positionY[0]!);
            currentZ = f(this.positionZ[i]! - this.positionZ[0]!);
            nx = f(nx + f(f(previousY - currentY) * f(previousZ + currentZ)));
            ny = f(ny + f(f(previousZ - currentZ) * f(previousX + currentX)));
            nz = f(nz + f(f(previousX - currentX) * f(previousY + currentY)));
        }

        const sum = new Vector3Df(nx, ny, nz);
        const localNorm = PlanarFace.norm(sum);
        if (localNorm < PlanarFace.EPSILON) {
            return false;
        }
        const unit = PlanarFace.inverseScaled(localNorm, sum, PlanarFace.EPSILON_FLOAT);
        this.normalX = unit.x();
        this.normalY = unit.y();
        this.normalZ = unit.z();
        return true;
    }

    /**
    Area and Jacobian J(u, v) = A + B.u + C.v of the mapping from the unit square
    (quadrilaterals) or the standard triangle (0, 0), (1, 0), (0, 1). The area of a
    quadrilateral is A + (B + C) / 2; B = C = 0 for parallelograms, and the
    coefficients are only kept when they are not negligible. Requires the normal
    */
    private computeAreaAndJacobian(): void {
        this.jacobianPresent = false;
        if (this.numberOfVertices === 3) {
            const d1 = PlanarFace.difference(this.getPosition(1), this.getPosition(0));
            const d2 = PlanarFace.difference(this.getPosition(2), this.getPosition(1));
            this.area = f(0.5 * PlanarFace.norm(PlanarFace.cross(d1, d2)));
            return;
        }

        const normal = this.getNormal();
        const d1 = PlanarFace.difference(this.getPosition(1), this.getPosition(0));
        const d2 = PlanarFace.difference(this.getPosition(2), this.getPosition(1));
        const d3 = PlanarFace.difference(this.getPosition(2), this.getPosition(3));
        const d4 = PlanarFace.difference(this.getPosition(3), this.getPosition(0));
        const a = PlanarFace.dot(PlanarFace.cross(d1, d4), normal);
        let b = PlanarFace.dot(PlanarFace.cross(d1, d3), normal);
        let c = PlanarFace.dot(PlanarFace.cross(d2, d4), normal);

        this.area = f(a + f(0.5 * f(b + c)));
        if (this.area < 0.0) {
            // Normal direction and vertex order are not consistent
            b = -b;
            c = -c;
            this.area = -this.area;
        }

        if (!(f(Math.abs(b) / this.area) < PlanarFace.EPSILON && f(Math.abs(c) / this.area) < PlanarFace.EPSILON)) {
            this.jacobianPresent = true;
            this.jacobianA = a;
            this.jacobianB = b;
            this.jacobianC = c;
        }
    }

    private computeMidPoint(): void {
        let x = 0.0;
        let y = 0.0;
        let z = 0.0;
        for (let i = 0; i < this.numberOfVertices; i++) {
            x = f(x + this.positionX[i]!);
            y = f(y + this.positionY[i]!);
            z = f(z + this.positionZ[i]!);
        }
        const mid = PlanarFace.inverseScaled(
            f(this.numberOfVertices),
            new Vector3Df(x, y, z),
            PlanarFace.EPSILON_FLOAT,
        );
        this.midPointX = mid.x();
        this.midPointY = mid.y();
        this.midPointZ = mid.z();
    }

    /**
    A "width" of the plane, for coplanarity tests: the largest distance of a
    vertex to the plane plus the vertex's position tolerance
    */
    private computeTolerance(): number {
        const normal = this.getNormal();
        let localTolerance = 0.0;
        for (let i = 0; i < this.numberOfVertices; i++) {
            const p = this.getPosition(i);
            const e = f(
                Math.abs(f(PlanarFace.dot(normal, p) + this.planeConstant)) +
                    PlanarFace.positionTolerance(p, PlanarFace.EPSILON_FLOAT),
            );
            if (e > localTolerance) {
                localTolerance = e;
            }
        }
        return localTolerance;
    }

    /**
    Index of the largest absolute component; ties go to x, then y. Used to turn
    point in polygon tests into 2D problems
    */
    private static dominantCoordinate(vector: Vector3Df): number {
        const ax = Math.abs(vector.x());
        const ay = Math.abs(vector.y());
        const az = Math.abs(vector.z());
        let indexValue = Math.max(ay, az);
        indexValue = Math.max(ax, f(indexValue));

        if (indexValue === ax) {
            return 0;
        }
        return indexValue === ay ? 1 : 2;
    }

    private static clipToUnitInterval(x: number): number {
        if (x < PlanarFace.EPSILON) {
            return PlanarFace.EPSILON;
        }
        return x > 1.0 - PlanarFace.EPSILON ? 1.0 - PlanarFace.EPSILON : x;
    }

    /**
    Looks for the solution of A.u^2 + B.u + C = 0 in [0, 1]; there must be exactly
    one. Returns false when there is no real solution, or both or none are in the
    interval (numerical inaccuracy); a best guess is returned then
    */
    private static solveQuadraticUnitInterval(A: number, B: number, C: number, x: number[]): boolean {
        const TOLERANCE = PlanarFace.TOLERANCE;
        let D = B * B - 4.0 * A * C;
        let x1: number;
        let x2: number;

        if (A < TOLERANCE && A > -TOLERANCE) {
            // Degenerate case, solve B*x + C = 0
            x1 = -1.0;
            x2 = -C / B;
        } else {
            if (D < -TOLERANCE * TOLERANCE) {
                x[0] = -B / (2 * A);
                Logger.reportMessage(
                    "Patch",
                    Logger.ERROR,
                    "",
                    JavaString.vformat(
                        "Bi-linear->Uniform mapping has negative discriminant D = %g.\nTaking 0 as discriminant and %g as solution.",
                        [D, x[0]],
                    ),
                );
                return false;
            }

            D = D > TOLERANCE * TOLERANCE ? Math.sqrt(D) : 0.0;
            A = 1.0 / (2.0 * A);
            x1 = (-B + D) * A;
            x2 = (-B - D) * A;

            if (x1 > -TOLERANCE && x1 < 1.0 + TOLERANCE) {
                x[0] = x1;
                // Both in the interval: ambiguous, x1 taken
                return !(x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE);
            }
        }

        if (x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE) {
            x[0] = x2;
            return true;
        }

        // May happen due to numerical errors: choose the root closest to [0, 1]
        let d: number;
        if (x1 > 1.0) {
            d = x1 - 1.0;
        } else {
            d = -x1;
        }
        x[0] = x1;
        if (x2 > 1.0) {
            if (x2 - 1.0 < d) {
                x[0] = x2;
            }
        } else {
            if (0.0 - x2 < d) {
                x[0] = x2;
            }
        }

        if (x[0]! < 0.0) {
            x[0] = 0.0;
        }
        if (x[0]! > 1.0) {
            x[0] = 1.0;
        }
        return false;
    }

    /**
    Barycentric (u, v) of a point in the triangle. Didier Badouel, Graphics
    Gems I, p390
    */
    private triangleParameters(point: Vector3Df, uv: Vector2Dd): boolean {
        const inside = this.parametersOfProjected(
            this.projectedA(point),
            this.projectedB(point),
            PlanarFace.parameterScratch,
        );
        uv.x = PlanarFace.parameterScratch[0]!;
        uv.y = PlanarFace.parameterScratch[1]!;
        return inside;
    }

    // Coordinates of a point projected along the dominant axis
    private projectedA(point: Vector3Df): number {
        return this.dominantAxis === 0 ? point.y() : point.x();
    }

    private projectedB(point: Vector3Df): number {
        return this.dominantAxis === 2 ? point.y() : point.z();
    }

    // (u, v) of a projected point written to uv[0], uv[1] by
    // triangleParametersOfProjected or quadrilateralParametersOfProjected,
    // scalars only (TypeScript: the Vector2Dd temporaries of the source were a
    // large share of the face test's time). Returns whether it is inside.
    private parametersOfProjected(pointA: number, pointB: number, uv: Float64Array): boolean {
        return this.numberOfVertices === 3
            ? this.triangleParametersOfProjected(pointA, pointB, uv)
            : this.quadrilateralParametersOfProjected(pointA, pointB, uv);
    }

    private static readonly parameterScratch = new Float64Array(2);

    // Projected position of vertex i, first and second coordinate
    private vertexA(i: number): number {
        return this.dominantAxis === 0 ? this.positionY[i]! : this.positionX[i]!;
    }

    private vertexB(i: number): number {
        return this.dominantAxis === 2 ? this.positionY[i]! : this.positionZ[i]!;
    }

    private triangleParametersOfProjected(pointA: number, pointB: number, uv: Float64Array): boolean {
        // (0, 0) when the point is rejected early, as the fresh Vector2Dd of
        // the source
        uv[0] = 0.0;
        uv[1] = 0.0;
        const u0 = this.vertexA(0);
        const v0 = this.vertexB(0);
        const p0x = pointA - u0;
        const p0y = pointB - v0;
        const p1x = this.vertexA(1) - u0;
        const p1y = this.vertexB(1) - v0;
        const p2x = this.vertexA(2) - u0;
        const p2y = this.vertexB(2) - v0;

        let alpha: number;
        let beta: number;
        if (p1x < -PlanarFace.EPSILON || p1x > PlanarFace.EPSILON) {
            beta = (p0y * p1x - p0x * p1y) / (p2y * p1x - p2x * p1y);
            if (beta >= 0.0 && beta <= 1.0) {
                alpha = (p0x - beta * p2x) / p1x;
            } else {
                return false;
            }
        } else {
            beta = p0x / p2x;
            if (beta >= 0.0 && beta <= 1.0) {
                alpha = (p0y - beta * p2y) / p1y;
            } else {
                return false;
            }
        }
        uv[0] = alpha;
        uv[1] = beta;
        return !(alpha < 0.0 || alpha + beta > 1.0);
    }

    /**
    Bi-linear (u, v) of a point in the quadrilateral. Christophe Schlick and
    Gilles Subrenat, "Ray Intersection of Tessellated Surfaces: Quadrangles versus
    Triangles", Graphics Gems V, pages 232-241
    */
    private quadrilateralParameters(point: Vector3Df, uv: Vector2Dd): boolean {
        const inside = this.quadrilateralParametersOfProjected(
            this.projectedA(point),
            this.projectedB(point),
            PlanarFace.parameterScratch,
        );
        uv.x = PlanarFace.parameterScratch[0]!;
        uv.y = PlanarFace.parameterScratch[1]!;
        return inside;
    }

    private quadrilateralParametersOfProjected(pointA: number, pointB: number, uv: Float64Array): boolean {
        const Ax = this.vertexA(0);
        const Ay = this.vertexB(0);
        const Bx = this.vertexA(1);
        const By = this.vertexB(1);
        const Cx = this.vertexA(2);
        const Cy = this.vertexB(2);
        const Dx = this.vertexA(3);
        const Dy = this.vertexB(3);

        const ABx = Bx - Ax;
        const ABy = By - Ay;
        const BCx = Cx - Bx;
        const BCy = Cy - By;
        const CDx = Dx - Cx;
        const CDy = Dy - Cy;
        const ADx = Dx - Ax;
        const ADy = Dy - Ay;
        const AEx = -(CDx + ABx);
        const AEy = -(CDy + ABy);
        const AMx = pointA - Ax;
        const AMy = pointB - Ay;
        // determinant(a, b) = a.x * b.y - a.y * b.x
        const det = (ax: number, ay: number, bx: number, by: number): number => ax * by - ay * bx;

        let u = -1.0;
        let v = -1.0;
        let isInside = false;
        let a: number;
        let b: number;
        let c: number;

        if (Math.abs(det(ABx, ABy, CDx, CDy)) < PlanarFace.EPSILON) {
            // AB parallel to CD
            const vx = ABx - CDx;
            const vy = ABy - CDy;
            v = det(AMx, AMy, vx, vy) / det(ADx, ADy, vx, vy);
            if (v >= 0.0 && v <= 1.0) {
                b = det(ABx, ABy, ADx, ADy) - det(AMx, AMy, AEx, AEy);
                c = det(AMx, AMy, ADx, ADy);
                u = Math.abs(b) < PlanarFace.EPSILON ? -1 : c / b;
                isInside = u >= 0.0 && u <= 1.0;
            }
        } else if (Math.abs(det(BCx, BCy, ADx, ADy)) < PlanarFace.EPSILON) {
            // AD parallel to BC
            const vx = ADx + BCx;
            const vy = ADy + BCy;
            u = det(AMx, AMy, vx, vy) / det(ABx, ABy, vx, vy);
            if (u >= 0.0 && u <= 1.0) {
                b = det(ADx, ADy, ABx, ABy) - det(AMx, AMy, AEx, AEy);
                c = det(AMx, AMy, ABx, ABy);
                v = Math.abs(b) < PlanarFace.EPSILON ? -1 : c / b;
                isInside = v >= 0.0 && v <= 1.0;
            }
        } else {
            // General case
            a = det(ABx, ABy, AEx, AEy);
            c = -det(AMx, AMy, ADx, ADy);
            b = det(ABx, ABy, ADx, ADy) - det(AMx, AMy, AEx, AEy);
            a = -0.5 / a;
            b *= a;
            c *= a + a;
            let sqrtDelta = b * b + c;
            if (sqrtDelta >= 0.0) {
                sqrtDelta = Math.sqrt(sqrtDelta);
                u = b - sqrtDelta;
                if (u < 0.0 || u > 1.0) {
                    // To choose u between 0 and 1
                    u = b + sqrtDelta;
                }
                if (u >= 0.0 && u <= 1.0) {
                    v = ADx + u * AEx;
                    if (Math.abs(v) < PlanarFace.EPSILON) {
                        v = (AMy - u * ABy) / (ADy + u * AEy);
                    } else {
                        v = (AMx - u * ABx) / v;
                    }
                    isInside = v >= 0.0 && v <= 1.0;
                }
            } else {
                u = v = -1.0;
            }
        }

        uv[0] = PlanarFace.clipToUnitInterval(u);
        uv[1] = PlanarFace.clipToUnitInterval(v);
        return isInside;
    }

    /**
    (u, v) of a point on the plane of the face, written to uv.x and uv.y;
    returns whether the point is inside. For points outside, (u, v) is not
    meaningful
    */
    public parameters(point: Vector3Df, uv: Vector2Dd): boolean {
        const localUv = new Vector2Dd();
        const inside =
            this.numberOfVertices === 3
                ? this.triangleParameters(point, localUv)
                : this.quadrilateralParameters(point, localUv);
        uv.x = localUv.x;
        uv.y = localUv.y;
        return inside;
    }

    /**
    Point at the given barycentric (triangle) or bi-linear (quadrilateral)
    parameters; for a triangle with u + v > 1, (1 - u, 1 - v) is used
    */
    public pointAt(u: number, v: number): Vector3Df {
        if (this.numberOfVertices === 3) {
            if (u + v > 1.0) {
                u = 1.0 - u;
                v = 1.0 - v;
            }
            return PlanarFace.pointInTriangle(
                this.getPosition(0),
                this.getPosition(1),
                this.getPosition(2),
                f(u),
                f(v),
            );
        }
        return PlanarFace.pointInQuadrilateral(
            this.getPosition(0),
            this.getPosition(1),
            this.getPosition(2),
            this.getPosition(3),
            f(u),
            f(v),
        );
    }

    /**
    Uniform coordinates are such that the area left of the line of points with
    u-coordinate u is u.A and the area under the line with v-coordinate v is v.A
    */
    public biLinearToUniform(uv: Vector2Dd): void {
        const a = this.jacobianA;
        const b = this.jacobianB;
        const c = this.jacobianC;
        uv.x = ((a + 0.5 * c + 0.5 * b * uv.x) * uv.x) / this.area;
        uv.y = ((a + 0.5 * b + 0.5 * c * uv.y) * uv.y) / this.area;
    }

    public uniformToBiLinear(uv: Vector2Dd): void {
        const a = this.jacobianA;
        const b = this.jacobianB;
        const c = this.jacobianC;
        const x: number[] = [0];
        let A = (0.5 * b) / this.area;
        let B = (a + 0.5 * c) / this.area;
        let C = -uv.x;
        x[0] = uv.x;
        PlanarFace.solveQuadraticUnitInterval(A, B, C, x);
        uv.x = x[0]!;

        A = (0.5 * c) / this.area;
        B = (a + 0.5 * b) / this.area;
        C = -uv.y;
        x[0] = uv.y;
        PlanarFace.solveQuadraticUnitInterval(A, B, C, x);
        uv.y = x[0]!;
    }

    // Area preserving mapping: uniform parameters to point, point to uniform
    // parameters
    public uniformPointAt(u: number, v: number): Vector3Df {
        const uv = new Vector2Dd(u, v);
        if (this.jacobianPresent) {
            this.uniformToBiLinear(uv);
        }
        return this.pointAt(uv.x, uv.y);
    }

    public uniformParameters(point: Vector3Df, uv: Vector2Dd): boolean {
        const inside = this.parameters(point, uv);
        if (this.jacobianPresent) {
            this.biLinearToUniform(uv);
        }
        return inside;
    }

    /**
    True when part of this face lies in front of the plane of the other face,
    beyond the other face's tolerance
    */
    public isAtLeastPartlyInFront(other: PlanarFace): boolean {
        const otherNormal = other.getNormal();
        for (let i = 0; i < this.numberOfVertices; i++) {
            const p = this.getPosition(i);
            const ep = f(PlanarFace.dot(otherNormal, p) + other.planeConstant);
            const localTolerance = f(other.tolerance + PlanarFace.positionTolerance(p, PlanarFace.EPSILON_FLOAT));
            if (ep > localTolerance) {
                return true;
            }
        }
        return false;
    }

    // True when each face is at least partly in front of the other
    public facing(other: PlanarFace): boolean {
        return this.isAtLeastPartlyInFront(other) && other.isAtLeastPartlyInFront(this);
    }

    // Signed distance of a point to the plane, in units of the normal
    public planeDistance(point: Vector3Df): number {
        return f(
            f(f(f(this.normalX * point.x()) + f(this.normalY * point.y())) + f(this.normalZ * point.z())) +
                this.planeConstant,
        );
    }

    /**
    Side of the face a ray with the given direction arrives at: FRONT when it
    travels against the normal, PARALLEL within EPSILON. The dot product of the
    normal with the direction is written to normalDotDirection[0]
    */
    public sideFacing(direction: Vector3Df, normalDotDirection: number[]): PlanarFace.Side {
        const d = f(
            f(f(this.normalX * direction.x()) + f(this.normalY * direction.y())) + f(this.normalZ * direction.z()),
        );
        normalDotDirection[0] = d;
        if (d > PlanarFace.EPSILON) {
            return PlanarFace.Side.BACK;
        }
        if (d < -PlanarFace.EPSILON) {
            return PlanarFace.Side.FRONT;
        }
        return PlanarFace.Side.PARALLEL;
    }

    /**
    Ray parameter of the hit with the plane, given the dot product of the
    normal with the ray direction (from sideFacing)
    */
    public planeHitParameter(origin: Vector3Df, normalDotDirection: number): number {
        return f(
            -f(
                f(f(f(this.normalX * origin.x()) + f(this.normalY * origin.y())) + f(this.normalZ * origin.z())) +
                    this.planeConstant,
            ) / normalDotDirection,
        );
    }

    /**
    One ray against the face, in one call: sideFacing, planeHitParameter, the
    hit point origin + distance * direction and its parameters. Returns true,
    with the hit stored, when the ray arrives at an accepted side, the distance
    lies in [minimumDistance, maximumDistance] and the point is inside the face;
    returns false otherwise (the hit is then unspecified)
    */
    public rayHit(
        origin: Vector3Df,
        direction: Vector3Df,
        acceptFront: boolean,
        acceptBack: boolean,
        minimumDistance: number,
        maximumDistance: number,
        hit: PlanarFace.RayHit,
    ): boolean {
        return this.rayHitComponents(
            origin.x(),
            origin.y(),
            origin.z(),
            direction.x(),
            direction.y(),
            direction.z(),
            acceptFront,
            acceptBack,
            minimumDistance,
            maximumDistance,
            hit,
        );
    }

    /**
    rayHit with the origin and direction given by their float components
    (TypeScript only: spares the caller two Vector3Df per ray). The
    components must already be floats (Math.fround).
    */
    public rayHitComponents(
        ox: number,
        oy: number,
        oz: number,
        dx: number,
        dy: number,
        dz: number,
        acceptFront: boolean,
        acceptBack: boolean,
        minimumDistance: number,
        maximumDistance: number,
        hit: PlanarFace.RayHit,
    ): boolean {
        // sideFacing
        const normalDotDirection = f(f(f(this.normalX * dx) + f(this.normalY * dy)) + f(this.normalZ * dz));
        let side: PlanarFace.Side;
        if (normalDotDirection > PlanarFace.EPSILON) {
            side = PlanarFace.Side.BACK;
        } else if (normalDotDirection < -PlanarFace.EPSILON) {
            side = PlanarFace.Side.FRONT;
        } else {
            side = PlanarFace.Side.PARALLEL;
        }
        if (
            side === PlanarFace.Side.PARALLEL ||
            (side === PlanarFace.Side.FRONT && !acceptFront) ||
            (side === PlanarFace.Side.BACK && !acceptBack)
        ) {
            return false;
        }
        // planeHitParameter
        const distance = f(
            -f(f(f(f(this.normalX * ox) + f(this.normalY * oy)) + f(this.normalZ * oz)) + this.planeConstant) /
                normalDotDirection,
        );
        if (distance > maximumDistance || distance < minimumDistance) {
            return false;
        }
        const pointX = f(ox + f(distance * dx));
        const pointY = f(oy + f(distance * dy));
        const pointZ = f(oz + f(distance * dz));
        hit.setSide(side);
        hit.setDistance(distance);
        hit.setPoint(pointX, pointY, pointZ);
        const uv = PlanarFace.parameterScratch;
        const inside = this.parametersOfProjected(
            this.dominantAxis === 0 ? pointY : pointX,
            this.dominantAxis === 2 ? pointY : pointZ,
            uv,
        );
        hit.setParameters(uv[0]!, uv[1]!);
        return inside;
    }
}

export namespace PlanarFace {
    export enum BuildStatus {
        VALID = 0,
        UNSUPPORTED_VERTEX_COUNT = 1,
        DEGENERATE_NORMAL = 2,
    }

    export enum Side {
        PARALLEL = 0,
        FRONT = 1,
        BACK = 2,
    }

    // Result of rayHit: the side the ray arrives at, the ray parameter of the
    // plane hit, the hit point and its (u, v) parameters
    export class RayHit {
        private side: Side = Side.PARALLEL;
        private distance = 0;
        private pointX = 0;
        private pointY = 0;
        private pointZ = 0;
        private u = 0;
        private v = 0;

        public getSide(): Side {
            return this.side;
        }

        public getDistance(): number {
            return this.distance;
        }

        public getPointX(): number {
            return this.pointX;
        }

        public getPointY(): number {
            return this.pointY;
        }

        public getPointZ(): number {
            return this.pointZ;
        }

        public getU(): number {
            return this.u;
        }

        public getV(): number {
            return this.v;
        }

        public setSide(inSide: Side): void {
            this.side = inSide;
        }

        public setDistance(inDistance: number): void {
            this.distance = inDistance;
        }

        public setPoint(x: number, y: number, z: number): void {
            this.pointX = x;
            this.pointY = y;
            this.pointZ = z;
        }

        public setParameters(inU: number, inV: number): void {
            this.u = inU;
            this.v = inV;
        }
    }
}
