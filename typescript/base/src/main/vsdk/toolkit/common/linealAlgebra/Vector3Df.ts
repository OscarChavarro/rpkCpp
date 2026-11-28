import { Float } from "../../../../java/lang/Float.js";
import { IllegalArgumentException } from "../../../../java/lang/IllegalArgumentException.js";
import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Vector3Dd } from "./Vector3Dd.js";
const f = Math.fround;
export class Vector3Df extends FundamentalEntity {
    private readonly xv: number;
    private readonly yv: number;
    private readonly zv: number;
    public constructor();
    public constructor(x: number, y: number, z: number);
    public constructor(other: Vector3Df | Vector3Dd);
    public constructor(a: number | Vector3Df | Vector3Dd = 0, b = 0, c = 0) {
        super();
        if (typeof a === "number") {
            this.xv = f(a);
            this.yv = f(b);
            this.zv = f(c);
        } else if (a instanceof Vector3Df) {
            this.xv = a.x();
            this.yv = a.y();
            this.zv = a.z();
        } else {
            this.xv = f(a.x());
            this.yv = f(a.y());
            this.zv = f(a.z());
        }
    }
    // Every operation rounds each product and each sum to single precision,
    // in the order of the C++ and Java float expressions, so that the three
    // ports give the same bits
    public multiply(a: number): Vector3Df {
        const af = f(a);
        return new Vector3Df(f(af * this.xv), f(af * this.yv), f(af * this.zv));
    }
    public crossProduct(o: Vector3Df): Vector3Df {
        return new Vector3Df(
            f(f(this.yv * o.zv) - f(this.zv * o.yv)),
            f(f(this.zv * o.xv) - f(this.xv * o.zv)),
            f(f(this.xv * o.yv) - f(this.yv * o.xv)),
        );
    }
    public dotProduct(o: Vector3Df): number {
        return f(f(f(this.xv * o.xv) + f(this.yv * o.yv)) + f(this.zv * o.zv));
    }
    public normalized(): Vector3Df;
    /**
    Scales by the inverse of the length, or returns this vector unchanged
    when the length is within [-epsilon, epsilon]. Unlike normalized(), the
    threshold applies to the length, and a unit vector is also rescaled
    */
    public normalized(epsilon: number): Vector3Df;
    public normalized(epsilon?: number): Vector3Df {
        if (epsilon !== undefined) return this.multiplyByInverse(this.length(), epsilon);
        let t = this.dotProduct(this);
        if (Math.abs(t) < f(VSDK.EPSILON)) return this;
        if (t !== 1) t = f(1 / Math.sqrt(t));
        return this.multiply(t);
    }
    public length(): number {
        return f(Math.sqrt(this.dotProduct(this)));
    }
    public add(o: Vector3Df): Vector3Df {
        return new Vector3Df(f(this.xv + o.xv), f(this.yv + o.yv), f(this.zv + o.zv));
    }
    public subtract(o: Vector3Df): Vector3Df {
        return new Vector3Df(f(this.xv - o.xv), f(this.yv - o.yv), f(this.zv - o.zv));
    }
    /**
    Square of the length (the dot product with itself)
    */
    public lengthSquared(): number {
        return f(f(f(this.xv * this.xv) + f(this.yv * this.yv)) + f(this.zv * this.zv));
    }
    /**
    Sum of the absolute values of the components (the L1 norm)
    */
    public manhattanLength(): number {
        return f(f(Vector3Df.absolute(this.xv) + Vector3Df.absolute(this.yv)) + Vector3Df.absolute(this.zv));
    }
    /**
    Distance to the given point: the length of (other - this)
    */
    public distance(other: Vector3Df): number {
        return other.subtract(this).length();
    }
    /**
    Square of the distance to the given point
    */
    public distanceSquared(other: Vector3Df): number {
        return other.subtract(this).lengthSquared();
    }
    /**
    Scales by 1 / s, or returns this vector unchanged when s is within
    [-epsilon, epsilon]. The factor 1 / s is computed once and multiplies
    each component
    */
    public multiplyByInverse(s: number, epsilon: number): Vector3Df {
        const sf = f(s);
        const epsilonf = f(epsilon);
        const factor = sf < -epsilonf || sf > epsilonf ? f(1 / sf) : 1;
        return new Vector3Df(f(factor * this.xv), f(factor * this.yv), f(factor * this.zv));
    }
    /**
    Index of the component of largest absolute value: 0 for x, 1 for y,
    2 for z; ties go to x before y and y before z
    */
    public dominantAxis(): number {
        const ax = Vector3Df.absolute(this.xv);
        const ay = Vector3Df.absolute(this.yv);
        const az = Vector3Df.absolute(this.zv);
        let m = ay > az ? ay : az;
        m = ax > m ? ax : m;
        if (m === ax) {
            return 0;
        }
        return m === ay ? 1 : 2;
    }
    public withX(x: number): Vector3Df {
        return new Vector3Df(x, this.yv, this.zv);
    }
    public withY(y: number): Vector3Df {
        return new Vector3Df(this.xv, y, this.zv);
    }
    public withZ(z: number): Vector3Df {
        return new Vector3Df(this.xv, this.yv, z);
    }
    public component(axis: number): number {
        return axis === 0 ? this.xv : axis === 1 ? this.yv : this.zv;
    }
    public x(): number {
        return this.xv;
    }
    public y(): number {
        return this.yv;
    }
    public z(): number {
        return this.zv;
    }
    public epsilonEquals(o: Vector3Df | null, e = f(VSDK.EPSILON)): boolean {
        if (o === null) return false;
        e = f(e);
        if (e < 0) throw new IllegalArgumentException("epsilon must be >= 0");
        return Math.abs(f(this.xv - o.xv)) <= e && Math.abs(f(this.yv - o.yv)) <= e && Math.abs(f(this.zv - o.zv)) <= e;
    }
    public equals(other: unknown): boolean {
        return (
            other instanceof Vector3Df &&
            Object.is(this.xv, other.xv) &&
            Object.is(this.yv, other.yv) &&
            Object.is(this.zv, other.zv)
        );
    }
    /**
    Java's `Vector3Df` inherits `Object.toString()`:
    `getClass().getName() + "@" + Integer.toHexString(hashCode())`. It is
    deterministic because the class overrides `hashCode()`.
    */
    public override toString(): string {
        return "vsdk.toolkit.common.linealAlgebra.Vector3Df@" + (this.hashCode() >>> 0).toString(16);
    }

    public hashCode(): number {
        let result = Float.hashCode(this.xv);
        result = (Math.imul(31, result) + Float.hashCode(this.yv)) | 0;
        return (Math.imul(31, result) + Float.hashCode(this.zv)) | 0;
    }
    private static absolute(a: number): number {
        return a < 0 ? -a : a;
    }
}
