import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Vector3Df } from "./Vector3Df.js";
import { Quaterniond } from "./Quaterniond.js";
const f = Math.fround;
export class Quaternionf extends FundamentalEntity {
    private readonly dv: Vector3Df;
    private readonly mv: number;
    public constructor();
    public constructor(d: Vector3Df, m: number);
    public constructor(o: Quaternionf | Quaterniond);
    public constructor(a: Vector3Df | Quaternionf | Quaterniond = new Vector3Df(), m = 0) {
        super();
        if (a instanceof Quaternionf) {
            this.dv = new Vector3Df(a.dv);
            this.mv = a.mv;
        } else if (a instanceof Quaterniond) {
            this.dv = new Vector3Df(a.direction());
            this.mv = f(a.magnitude());
        } else {
            if (a === null) throw new TypeError("Quaternion direction cannot be null");
            this.dv = new Vector3Df(a);
            this.mv = f(m);
        }
    }
    public static copyOf(o: Quaternionf): Quaternionf {
        if (o === null) throw new TypeError("Quaternion to copy cannot be null");
        return new Quaternionf(o);
    }
    public lengthSquared(): number {
        return f(f(this.mv * this.mv) + this.dv.dotProduct(this.dv));
    }
    public length(): number {
        return f(Math.sqrt(this.lengthSquared()));
    }
    public normalized(): Quaternionf {
        const l = this.length();
        return Math.abs(l) < f(VSDK.EPSILON) ? this : new Quaternionf(this.dv.multiply(f(1 / l)), f(this.mv / l));
    }
    public conjugated(): Quaternionf {
        return new Quaternionf(this.dv.multiply(-1), this.mv);
    }
    public rotate(v: Vector3Df): Vector3Df {
        if (v === null) throw new TypeError("Vector to rotate cannot be null");
        const uv = this.dv.crossProduct(v);
        return v.add(uv.multiply(f(2 * this.mv))).add(this.dv.crossProduct(uv).multiply(2));
    }
    public withDirection(d: Vector3Df): Quaternionf {
        return new Quaternionf(d, this.mv);
    }
    public withMagnitude(m: number): Quaternionf {
        return new Quaternionf(this.dv, m);
    }
    public direction(): Vector3Df {
        return this.dv;
    }
    public magnitude(): number {
        return this.mv;
    }
    public epsilonEquals(o: Quaternionf | null, e = f(VSDK.EPSILON)): boolean {
        if (o === null) return false;
        if (e < 0) throw new RangeError("epsilon must be >= 0");
        return this.dv.epsilonEquals(o.dv, e) && Math.abs(this.mv - o.mv) <= e;
    }
    public override toString(): string {
        return `${this.dv.toString()} / ${VSDK.formatDouble(this.mv)}`;
    }
}
