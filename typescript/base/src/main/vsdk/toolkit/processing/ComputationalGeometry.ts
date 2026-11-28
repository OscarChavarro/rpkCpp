import { ProcessingElement } from "./ProcessingElement.js";
import { VSDK } from "../common/VSDK.js";
import { Vector2Dd } from "../common/linealAlgebra/Vector2Dd.js";
import { Vector3Dd } from "../common/linealAlgebra/Vector3Dd.js";
import { Containment, containmentValue } from "./Containment.js";

/**
Java's nested class `ComputationalGeometry.ClippedLine2DResult` (Java's
`Vector2Dd.x()` accessor is the `x` field here). It is also
reachable as `ComputationalGeometry.ClippedLine2DResult`.
*/
export class ClippedLine2DResult {
    private readonly acceptedValue: boolean;
    private readonly clipped0Value: Vector2Dd;
    private readonly clipped1Value: Vector2Dd;

    public constructor(accepted: boolean, clipped0: Vector2Dd, clipped1: Vector2Dd) {
        this.acceptedValue = accepted;
        this.clipped0Value = clipped0;
        this.clipped1Value = clipped1;
    }

    public accepted(): boolean {
        return this.acceptedValue;
    }

    public clipped0(): Vector2Dd {
        return this.clipped0Value;
    }

    public clipped1(): Vector2Dd {
        return this.clipped1Value;
    }
}

/**
This class contains common computational geometry operations (mostly
geometrical querys over existing geometries). This is a companion class
for the `GeometricModeler`, which holds creation and modification operations
over gemetries.
*/
export class ComputationalGeometry extends ProcessingElement {
    public static readonly ClippedLine2DResult = ClippedLine2DResult;

    private static readonly COHEN_SUTHERLAND_2D_INSIDE = 0; // 0000
    private static readonly COHEN_SUTHERLAND_2D_LEFT = 1; // 0001
    private static readonly COHEN_SUTHERLAND_2D_RIGHT = 2; // 0010
    private static readonly COHEN_SUTHERLAND_2D_BOTTOM = 4; // 0100
    private static readonly COHEN_SUTHERLAND_2D_TOP = 8; // 1000

    /**
    Given a line from p0 to p1 and a point p, this method gives the minimum
    distance between line and point.
    Following implementation from http://geomalgorithms.com/a02-_lines.html
    */
    public static lineToPointDistance(p0: Vector3Dd, p1: Vector3Dd, p: Vector3Dd): number {
        const lineVector: Vector3Dd = p1.subtract(p0);
        const denominator: number = lineVector.length();

        if (denominator < VSDK.EPSILON) {
            return Number.NaN;
        }

        let v: Vector3Dd, w: Vector3Dd;
        v = p1.subtract(p0);
        w = p.subtract(p0);

        let c1: number;
        let c2: number;

        c1 = w.dotProduct(v);
        if (c1 <= 0.0) {
            return Vector3Dd.distance(p, p0);
        }

        c2 = v.dotProduct(v);
        if (c2 <= c1) {
            return Vector3Dd.distance(p, p1);
        }

        let b: number;
        b = c1 / c2;
        let pb: Vector3Dd;
        pb = p0.add(v.multiply(b));
        return Vector3Dd.distance(p, pb);
    }

    /**
    Given a line that passes between points `p0` and `p1`, this method
    determines if point `p` falls under `distanceTolerance` in such line.
    Returns Java's `Containment.X.value()` integer code.
    */
    public static lineContainmentTest(p0: Vector3Dd, p1: Vector3Dd, p: Vector3Dd, distanceTolerance: number): number {
        let d: number;

        d = ComputationalGeometry.lineToPointDistance(p0, p1, p);

        if (d <= distanceTolerance) {
            return containmentValue(Containment.LIMIT);
        }
        return containmentValue(Containment.OUTSIDE);
    }

    /**
    Given a line that passes between points `p0` and `p1`, this method
    determines if point `p` falls under `distanceTolerance` in such line.

    This method is functionaly equivalent to procedures `contev` and
    `intrev` from program [MANT1988].13.3. and section [MANT1988].13.2.2.
    Returns Java's `Containment.X.value()` integer code.
    */
    public static lineSegmentContainmentTest(
        p0: Vector3Dd,
        p1: Vector3Dd,
        p: Vector3Dd,
        distanceTolerance: number,
    ): number {
        let d: number;
        let a: Vector3Dd, b: Vector3Dd;

        a = p1.subtract(p0);
        b = p.subtract(p0);

        const denominator: number = a.length();
        if (denominator < VSDK.EPSILON) return containmentValue(Containment.OUTSIDE);
        const numerator: number = a.crossProduct(b).length();
        d = numerator / denominator;

        if (d <= distanceTolerance) {
            const t: number = a.dotProduct(b) / a.dotProduct(a);
            if (t < -VSDK.EPSILON || t > 1 + VSDK.EPSILON) return containmentValue(Containment.OUTSIDE);
            return containmentValue(Containment.LIMIT);
        }
        return containmentValue(Containment.OUTSIDE);
    }

    /**
    Implementation of the Cohen-Sutherland line clipping algorithm on two
    Dimensions
    @param p0 first point of the line to be clipped
    @param p1 second point of the line to be clipped
    @param min coordinate with the minimum (x,y) values of the clipping
    rectangle
    @param max coordinate with the maximum (x,y) values of the clipping
    rectangle
    @param tolerance could be 0, or an small value. Positive makes area smaller,
    negative makes area bigger.
    @return `true` when there is at least a portion of the line inside the
    clipping rectangle and `false` when the line is outside the clipping
    rectangle.
    */
    public static cohenSutherlandLineClipping2D(
        p0: Vector2Dd,
        p1: Vector2Dd,
        min: Vector2Dd,
        max: Vector2Dd,
        tolerance: number,
    ): boolean {
        return ComputationalGeometry.cohenSutherlandLineClipping2DResult(p0, p1, min, max, tolerance).accepted();
    }

    public static cohenSutherlandLineClipping2DResult(
        p0: Vector2Dd,
        p1: Vector2Dd,
        min: Vector2Dd,
        max: Vector2Dd,
        tolerance: number,
    ): ClippedLine2DResult {
        let clipped0: Vector2Dd = p0;
        let clipped1: Vector2Dd = p1;
        const minX: number = Math.min(min.x, max.x);
        const maxX: number = Math.max(min.x, max.x);
        const minY: number = Math.min(min.y, max.y);
        const maxY: number = Math.max(min.y, max.y);
        const clipMin: Vector2Dd = new Vector2Dd(minX, minY);
        const clipMax: Vector2Dd = new Vector2Dd(maxX, maxY);

        if (maxX - minX < VSDK.EPSILON || maxY - minY < VSDK.EPSILON) {
            return new ClippedLine2DResult(false, p0, p1);
        }

        let outCode0: number = ComputationalGeometry.computeCohenSutherland2DCode(
            clipped0,
            clipMin,
            clipMax,
            tolerance,
        );
        let outCode1: number = ComputationalGeometry.computeCohenSutherland2DCode(
            clipped1,
            clipMin,
            clipMax,
            tolerance,
        );
        let outCodeOut: number;
        let accept = false;
        let x = 0.0;
        let y = 0.0;

        while (!accept) {
            if ((outCode0 | outCode1) === 0) {
                accept = true;
                break;
            } else if ((outCode0 & outCode1) !== 0) {
                accept = false;
                break;
            } else {
                const e: number = 2 * tolerance; //2*VSDK.EPSILON;
                outCodeOut = outCode0 !== 0 ? outCode0 : outCode1;
                if (
                    (outCodeOut & ComputationalGeometry.COHEN_SUTHERLAND_2D_TOP) ===
                    ComputationalGeometry.COHEN_SUTHERLAND_2D_TOP
                ) {
                    x =
                        clipped0.x +
                        ((clipped1.x - clipped0.x) * (clipMax.y - e - clipped0.y)) / (clipped1.y - clipped0.y);
                    y = clipMax.y - e;
                } else if (
                    (outCodeOut & ComputationalGeometry.COHEN_SUTHERLAND_2D_BOTTOM) ===
                    ComputationalGeometry.COHEN_SUTHERLAND_2D_BOTTOM
                ) {
                    x =
                        clipped0.x +
                        ((clipped1.x - clipped0.x) * (clipMin.y + e - clipped0.y)) / (clipped1.y - clipped0.y);
                    y = clipMin.y + e;
                } else if (
                    (outCodeOut & ComputationalGeometry.COHEN_SUTHERLAND_2D_RIGHT) ===
                    ComputationalGeometry.COHEN_SUTHERLAND_2D_RIGHT
                ) {
                    y =
                        clipped0.y +
                        ((clipped1.y - clipped0.y) * (clipMax.x - e - clipped0.x)) / (clipped1.x - clipped0.x);
                    x = clipMax.x - e;
                } else if (
                    (outCodeOut & ComputationalGeometry.COHEN_SUTHERLAND_2D_LEFT) ===
                    ComputationalGeometry.COHEN_SUTHERLAND_2D_LEFT
                ) {
                    y =
                        clipped0.y +
                        ((clipped1.y - clipped0.y) * (clipMin.x + e - clipped0.x)) / (clipped1.x - clipped0.x);
                    x = clipMin.x + e;
                }
            }
            if (outCodeOut === outCode0) {
                clipped0 = new Vector2Dd(x, y);
                outCode0 = ComputationalGeometry.computeCohenSutherland2DCode(clipped0, clipMin, clipMax, tolerance);
            } else {
                clipped1 = new Vector2Dd(x, y);
                outCode1 = ComputationalGeometry.computeCohenSutherland2DCode(clipped1, clipMin, clipMax, tolerance);
            }
        }
        return new ClippedLine2DResult(accept, clipped0, clipped1);
    }

    /**
    Compute a code of the position of a point relative to the clipping
    rectangle
    @return the code in four bits representing the location of the point
    */
    private static computeCohenSutherland2DCode(
        p: Vector2Dd,
        min: Vector2Dd,
        max: Vector2Dd,
        tolerance: number,
    ): number {
        let outCode: number;

        outCode = ComputationalGeometry.COHEN_SUTHERLAND_2D_INSIDE;
        const e: number = tolerance;

        if (p.x < min.x + e) {
            outCode |= ComputationalGeometry.COHEN_SUTHERLAND_2D_LEFT;
        }
        if (p.x > max.x - e) {
            outCode |= ComputationalGeometry.COHEN_SUTHERLAND_2D_RIGHT;
        }
        if (p.y < min.y + e) {
            outCode |= ComputationalGeometry.COHEN_SUTHERLAND_2D_BOTTOM;
        }
        if (p.y > max.y - e) {
            outCode |= ComputationalGeometry.COHEN_SUTHERLAND_2D_TOP;
        }
        return outCode;
    }
}
