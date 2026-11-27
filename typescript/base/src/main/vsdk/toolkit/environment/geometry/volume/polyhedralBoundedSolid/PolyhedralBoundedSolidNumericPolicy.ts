//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { VSDK } from "../../../../common/VSDK.js";
import type { Vector2Dd } from "../../../../common/linealAlgebra/Vector2Dd.js";
import { Vector3Dd } from "../../../../common/linealAlgebra/Vector3Dd.js";
import type { _PolyhedralBoundedSolidFace } from "./nodes/_PolyhedralBoundedSolidFace.js";
import { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";

export class ToleranceContext {
    private readonly modelScaleValue: number;
    private readonly epsilonValue: number;
    private readonly bigEpsilonValue: number;
    private readonly unitVectorToleranceValue: number;
    private readonly angleToleranceValue: number;
    private readonly coplanarDotToleranceValue: number;
    private readonly unitIntervalToleranceValue: number;

    public constructor(
        modelScale: number,
        epsilon: number,
        bigEpsilon: number,
        unitVectorTolerance: number,
        angleTolerance: number,
        coplanarDotTolerance: number,
        unitIntervalTolerance: number,
    ) {
        this.modelScaleValue = modelScale;
        this.epsilonValue = epsilon;
        this.bigEpsilonValue = bigEpsilon;
        this.unitVectorToleranceValue = unitVectorTolerance;
        this.angleToleranceValue = angleTolerance;
        this.coplanarDotToleranceValue = coplanarDotTolerance;
        this.unitIntervalToleranceValue = unitIntervalTolerance;
    }

    public modelScale(): number {
        return this.modelScaleValue;
    }

    public epsilon(): number {
        return this.epsilonValue;
    }

    public bigEpsilon(): number {
        return this.bigEpsilonValue;
    }

    public unitVectorTolerance(): number {
        return this.unitVectorToleranceValue;
    }

    public angleTolerance(): number {
        return this.angleToleranceValue;
    }

    public coplanarDotTolerance(): number {
        return this.coplanarDotToleranceValue;
    }

    public unitIntervalTolerance(): number {
        return this.unitIntervalToleranceValue;
    }
}

type _ToleranceContext = ToleranceContext;

/**
Numerical tolerances for geometric predicates used to implement the face
equations and intersection predicates of chapter [MANT1988].13 and the robust
case distinctions required by chapter [MANT1988].15.
*/
export class PolyhedralBoundedSolidNumericPolicy {
    public static readonly BREP_EPSILON = VSDK.EPSILON;
    public static readonly BREP_BIG_EPSILON = 10.0 * PolyhedralBoundedSolidNumericPolicy.BREP_EPSILON;

    /** Scale of an empty or degenerate model (no extent to measure). */
    private static readonly DEFAULT_SCALE = 1.0;
    /**
    Smallest model scale. Tolerances follow the model down to this size, so
    a part modelled in metres at 10^-3 gets 10^-3 times the tolerances of
    the same part at unit size. They used to stop at scale 1: below it the
    tolerances were absolute and, at 10^-3, a tenth of the features.
    */
    private static readonly MIN_SCALE = 1.0e-9;
    private static readonly MAX_UNIT_INTERVAL_TOLERANCE = 1.0e-3;
    private static readonly DEFAULT_CONTEXT = PolyhedralBoundedSolidNumericPolicy.fromScale(
        PolyhedralBoundedSolidNumericPolicy.DEFAULT_SCALE,
    );

    private constructor() {}

    public static defaultContext(): ToleranceContext {
        return PolyhedralBoundedSolidNumericPolicy.DEFAULT_CONTEXT;
    }

    public static fromScale(modelScale: number): ToleranceContext {
        const safeScale = PolyhedralBoundedSolidNumericPolicy.sanitizeScale(modelScale);
        const eps = PolyhedralBoundedSolidNumericPolicy.BREP_EPSILON * safeScale;
        const bigEps = PolyhedralBoundedSolidNumericPolicy.BREP_BIG_EPSILON * safeScale;
        const unitTol = PolyhedralBoundedSolidNumericPolicy.BREP_BIG_EPSILON;
        const angleTol = PolyhedralBoundedSolidNumericPolicy.BREP_BIG_EPSILON;
        const coplanarDotTol = 10.0 * PolyhedralBoundedSolidNumericPolicy.BREP_BIG_EPSILON;
        const unitIntervalTol = PolyhedralBoundedSolidNumericPolicy.clamp(
            bigEps / safeScale,
            PolyhedralBoundedSolidNumericPolicy.BREP_BIG_EPSILON,
            PolyhedralBoundedSolidNumericPolicy.MAX_UNIT_INTERVAL_TOLERANCE,
        );
        return new ToleranceContext(safeScale, eps, bigEps, unitTol, angleTol, coplanarDotTol, unitIntervalTol);
    }

    public static forSolid(solid: PolyhedralBoundedSolid | null): ToleranceContext {
        return PolyhedralBoundedSolidNumericPolicy.fromScale(
            PolyhedralBoundedSolidNumericPolicy.estimateSolidScale(solid),
        );
    }

    public static forSolids(a: PolyhedralBoundedSolid | null, b: PolyhedralBoundedSolid | null): ToleranceContext {
        return PolyhedralBoundedSolidNumericPolicy.fromScale(
            Math.max(
                PolyhedralBoundedSolidNumericPolicy.estimateSolidScale(a),
                PolyhedralBoundedSolidNumericPolicy.estimateSolidScale(b),
            ),
        );
    }

    public static forFace(face: _PolyhedralBoundedSolidFace | null): ToleranceContext {
        return PolyhedralBoundedSolidNumericPolicy.fromScale(
            PolyhedralBoundedSolidNumericPolicy.estimateFaceScale(face),
        );
    }

    public static forPoints(points: readonly (Vector3Dd | null)[] | null): ToleranceContext {
        return PolyhedralBoundedSolidNumericPolicy.fromScale(
            PolyhedralBoundedSolidNumericPolicy.estimatePointsScale(points),
        );
    }

    private static sanitizeScale(scale: number): number {
        if (!Number.isFinite(scale) || scale <= 0.0) {
            return PolyhedralBoundedSolidNumericPolicy.DEFAULT_SCALE;
        }
        if (scale < PolyhedralBoundedSolidNumericPolicy.MIN_SCALE) {
            return PolyhedralBoundedSolidNumericPolicy.MIN_SCALE;
        }
        return scale;
    }

    private static estimateSolidScale(solid: PolyhedralBoundedSolid | null): number {
        if (solid === null) {
            return PolyhedralBoundedSolidNumericPolicy.DEFAULT_SCALE;
        }

        const minMax = solid.getMinMax();
        if (minMax === null || minMax.length < 6) {
            return PolyhedralBoundedSolidNumericPolicy.DEFAULT_SCALE;
        }
        return PolyhedralBoundedSolidNumericPolicy.diagonalSize(
            minMax[0]!,
            minMax[1]!,
            minMax[2]!,
            minMax[3]!,
            minMax[4]!,
            minMax[5]!,
        );
    }

    private static estimateFaceScale(face: _PolyhedralBoundedSolidFace | null): number {
        if (face === null || face.boundariesList === null) {
            return PolyhedralBoundedSolidNumericPolicy.DEFAULT_SCALE;
        }

        let minX = Number.POSITIVE_INFINITY;
        let minY = Number.POSITIVE_INFINITY;
        let minZ = Number.POSITIVE_INFINITY;
        let maxX = -Number.POSITIVE_INFINITY;
        let maxY = -Number.POSITIVE_INFINITY;
        let maxZ = -Number.POSITIVE_INFINITY;
        let found = false;

        let i: number;
        let j: number;
        for (i = 0; i < face.boundariesList.size(); i++) {
            const loop = face.boundariesList.get(i);
            if (loop === null) {
                continue;
            }
            for (j = 0; j < loop.halfEdgesList.size(); j++) {
                const he = loop.halfEdgesList.get(j);
                if (he === null || he.startingVertex === null) {
                    continue;
                }
                const p = he.startingVertex.position;
                if (p === null) {
                    continue;
                }
                found = true;
                if (p.x() < minX) minX = p.x();
                if (p.y() < minY) minY = p.y();
                if (p.z() < minZ) minZ = p.z();
                if (p.x() > maxX) maxX = p.x();
                if (p.y() > maxY) maxY = p.y();
                if (p.z() > maxZ) maxZ = p.z();
            }
        }

        if (!found) {
            return PolyhedralBoundedSolidNumericPolicy.DEFAULT_SCALE;
        }
        return PolyhedralBoundedSolidNumericPolicy.diagonalSize(minX, minY, minZ, maxX, maxY, maxZ);
    }

    private static estimatePointsScale(points: readonly (Vector3Dd | null)[] | null): number {
        if (points === null || points.length < 2) {
            return PolyhedralBoundedSolidNumericPolicy.DEFAULT_SCALE;
        }

        let minX = Number.POSITIVE_INFINITY;
        let minY = Number.POSITIVE_INFINITY;
        let minZ = Number.POSITIVE_INFINITY;
        let maxX = -Number.POSITIVE_INFINITY;
        let maxY = -Number.POSITIVE_INFINITY;
        let maxZ = -Number.POSITIVE_INFINITY;
        let found = false;

        let i: number;
        for (i = 0; i < points.length; i++) {
            const p = points[i]!;
            if (p === null) {
                continue;
            }
            found = true;
            if (p.x() < minX) minX = p.x();
            if (p.y() < minY) minY = p.y();
            if (p.z() < minZ) minZ = p.z();
            if (p.x() > maxX) maxX = p.x();
            if (p.y() > maxY) maxY = p.y();
            if (p.z() > maxZ) maxZ = p.z();
        }

        if (!found) {
            return PolyhedralBoundedSolidNumericPolicy.DEFAULT_SCALE;
        }
        return PolyhedralBoundedSolidNumericPolicy.diagonalSize(minX, minY, minZ, maxX, maxY, maxZ);
    }

    private static diagonalSize(
        minX: number,
        minY: number,
        minZ: number,
        maxX: number,
        maxY: number,
        maxZ: number,
    ): number {
        const dx = maxX - minX;
        const dy = maxY - minY;
        const dz = maxZ - minZ;
        const diag = Math.sqrt(dx * dx + dy * dy + dz * dz);
        return PolyhedralBoundedSolidNumericPolicy.sanitizeScale(diag);
    }

    /**
    Java overloads `compare(double, double, double)` and
    `compare(double, double, ToleranceContext)`.
    */
    public static compare(a: number, b: number, tolerance: number | ToleranceContext): number {
        if (typeof tolerance !== "number") {
            return PolyhedralBoundedSolidNumericPolicy.compare(a, b, tolerance.epsilon());
        }
        return PolyhedralBoundedSolid.compareValue(a, b, tolerance);
    }

    public static compareToZero(value: number, context: ToleranceContext): number {
        return PolyhedralBoundedSolidNumericPolicy.compare(value, 0.0, context.epsilon());
    }

    public static compareToZeroBig(value: number, context: ToleranceContext): number {
        return PolyhedralBoundedSolidNumericPolicy.compare(value, 0.0, context.bigEpsilon());
    }

    public static isZero(value: number, context: ToleranceContext): boolean {
        return Math.abs(value) <= context.epsilon();
    }

    public static isZeroBig(value: number, context: ToleranceContext): boolean {
        return Math.abs(value) <= context.bigEpsilon();
    }

    public static pointsCoincident(a: Vector3Dd, b: Vector3Dd, context: ToleranceContext): boolean {
        return Vector3Dd.distance(a, b) <= context.bigEpsilon();
    }

    public static pointsSeparated(a: Vector3Dd, b: Vector3Dd, context: ToleranceContext): boolean {
        return Vector3Dd.distance(a, b) > context.bigEpsilon();
    }

    public static testPointInside(
        face: _PolyhedralBoundedSolidFace,
        point: Vector3Dd,
        context: ToleranceContext,
    ): number {
        return face.testPointInside(point, context.bigEpsilon());
    }

    public static vectorsColinear(a: Vector3Dd, b: Vector3Dd, _context: ToleranceContext): boolean {
        // |a x b| = |a| |b| sin(angle): compare the sine, so that the test
        // does not depend on the length of the vectors (it used to accept
        // angles up to bigEpsilon / length, half a degree at 10^-3).
        return (
            a.crossProduct(b).length() <= PolyhedralBoundedSolidNumericPolicy.BREP_BIG_EPSILON * a.length() * b.length()
        );
    }

    public static unitVectorsParallel(a: Vector3Dd, b: Vector3Dd, context: ToleranceContext): boolean {
        return a.crossProduct(b).length() <= context.unitVectorTolerance();
    }

    public static angleIntervalsOverlap(upperA: number, lowerB: number, context: ToleranceContext): boolean {
        const tolerance = context.angleTolerance();
        return upperA + tolerance > lowerB - tolerance;
    }

    public static unitIntervalContainsStrictly(t: number, context: ToleranceContext): boolean {
        return t > context.unitIntervalTolerance() && t < 1.0 - context.unitIntervalTolerance();
    }

    public static orientationTolerance2D(a: Vector2Dd, b: Vector2Dd, c: Vector2Dd, context: ToleranceContext): number {
        const lx = Math.max(Math.max(Math.abs(a.x - b.x), Math.abs(a.x - c.x)), Math.abs(b.x - c.x));
        const ly = Math.max(Math.max(Math.abs(a.y - b.y), Math.abs(a.y - c.y)), Math.abs(b.y - c.y));
        // The orientation is a cross product, |b - a| times the distance of
        // c from the line: a distance tolerance times the span. The span
        // used to be at least 1, which below unit size made the tolerance
        // an angle of bigEpsilon / span (half a degree at 10^-3).
        const span = Math.max(lx, ly);
        return context.bigEpsilon() * span;
    }

    public static linearTolerance2D(context: ToleranceContext): number {
        return context.bigEpsilon();
    }

    public static areaTolerance2D(context: ToleranceContext): number {
        const linearTolerance = context.bigEpsilon();
        return linearTolerance * linearTolerance;
    }

    private static clamp(value: number, min: number, max: number): number {
        if (value < min) {
            return min;
        }
        if (value > max) {
            return max;
        }
        return value;
    }
}

export namespace PolyhedralBoundedSolidNumericPolicy {
    export type ToleranceContext = _ToleranceContext;
}
