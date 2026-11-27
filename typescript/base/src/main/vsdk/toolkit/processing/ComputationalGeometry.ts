import { ProcessingElement } from "./ProcessingElement.js";
import { VSDK } from "../common/VSDK.js";
import { Vector2Dd } from "../common/linealAlgebra/Vector2Dd.js";
import { Vector3Dd } from "../common/linealAlgebra/Vector3Dd.js";
import { Containment } from "./Containment.js";
export class ClippedLine2DResult {
    public constructor(
        private readonly acceptedValue: boolean,
        private readonly clipped0Value: Vector2Dd,
        private readonly clipped1Value: Vector2Dd,
    ) {}
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
export class ComputationalGeometry extends ProcessingElement {
    public static lineToPointDistance(a: Vector3Dd, b: Vector3Dd, p: Vector3Dd): number {
        const v = b.subtract(a);
        if (v.length() < VSDK.EPSILON) return NaN;
        const w = p.subtract(a),
            c1 = w.dotProduct(v);
        if (c1 <= 0) return Vector3Dd.distance(p, a);
        const c2 = v.dotProduct(v);
        if (c2 <= c1) return Vector3Dd.distance(p, b);
        return Vector3Dd.distance(p, a.add(v.multiply(c1 / c2)));
    }
    public static lineContainmentTest(a: Vector3Dd, b: Vector3Dd, p: Vector3Dd, t: number): Containment {
        return this.lineToPointDistance(a, b, p) <= t ? Containment.LIMIT : Containment.OUTSIDE;
    }
    public static lineSegmentContainmentTest(a: Vector3Dd, b: Vector3Dd, p: Vector3Dd, t: number): Containment {
        const v = b.subtract(a),
            w = p.subtract(a);
        if (v.length() < VSDK.EPSILON) return Containment.OUTSIDE;
        if (v.crossProduct(w).length() / v.length() > t) return Containment.OUTSIDE;
        const q = v.dotProduct(w) / v.dotProduct(v);
        return q < -VSDK.EPSILON || q > 1 + VSDK.EPSILON ? Containment.OUTSIDE : Containment.LIMIT;
    }
    public static cohenSutherlandLineClipping2D(
        a: Vector2Dd,
        b: Vector2Dd,
        min: Vector2Dd,
        max: Vector2Dd,
        t: number,
    ): boolean {
        return this.cohenSutherlandLineClipping2DResult(a, b, min, max, t).accepted();
    }
    public static cohenSutherlandLineClipping2DResult(
        a: Vector2Dd,
        b: Vector2Dd,
        min: Vector2Dd,
        max: Vector2Dd,
        t: number,
    ): ClippedLine2DResult {
        let p = a,
            q = b;
        const lo = new Vector2Dd(Math.min(min.x, max.x), Math.min(min.y, max.y)),
            hi = new Vector2Dd(Math.max(min.x, max.x), Math.max(min.y, max.y));
        if (hi.x - lo.x < VSDK.EPSILON || hi.y - lo.y < VSDK.EPSILON) return new ClippedLine2DResult(false, a, b);
        const code = (v: Vector2Dd) =>
            (v.x < lo.x + t ? 1 : 0) | (v.x > hi.x - t ? 2 : 0) | (v.y < lo.y + t ? 4 : 0) | (v.y > hi.y - t ? 8 : 0);
        let cp = code(p),
            cq = code(q);
        while (true) {
            if ((cp | cq) === 0) return new ClippedLine2DResult(true, p, q);
            if ((cp & cq) !== 0) return new ClippedLine2DResult(false, p, q);
            const c = cp || cq,
                e = 2 * t;
            let x = 0,
                y = 0;
            if (c & 8) {
                x = p.x + ((q.x - p.x) * (hi.y - e - p.y)) / (q.y - p.y);
                y = hi.y - e;
            } else if (c & 4) {
                x = p.x + ((q.x - p.x) * (lo.y + e - p.y)) / (q.y - p.y);
                y = lo.y + e;
            } else if (c & 2) {
                y = p.y + ((q.y - p.y) * (hi.x - e - p.x)) / (q.x - p.x);
                x = hi.x - e;
            } else {
                y = p.y + ((q.y - p.y) * (lo.x + e - p.x)) / (q.x - p.x);
                x = lo.x + e;
            }
            if (c === cp) {
                p = new Vector2Dd(x, y);
                cp = code(p);
            } else {
                q = new Vector2Dd(x, y);
                cq = code(q);
            }
        }
    }
}
