import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Vector3Dd } from "./Vector3Dd.js";
export class Quaterniond extends FundamentalEntity {
    private readonly dv: Vector3Dd;
    private readonly mv: number;
    public constructor();
    public constructor(d: Vector3Dd, m: number);
    public constructor(o: Quaterniond);
    public constructor(a: Vector3Dd | Quaterniond = new Vector3Dd(), m = 0) {
        super();
        if (a instanceof Quaterniond) {
            this.dv = Vector3Dd.copyOf(a.dv);
            this.mv = a.mv;
        } else {
            if (a === null) throw new TypeError("Quaterniond direction cannot be null");
            this.dv = Vector3Dd.copyOf(a);
            this.mv = m;
        }
    }
    public static copyOf(o: Quaterniond): Quaterniond {
        if (o === null) throw new TypeError("Quaterniond to copy cannot be null");
        return new Quaterniond(o);
    }
    public lengthSquared(): number {
        return this.mv * this.mv + this.dv.dotProduct(this.dv);
    }
    public length(): number {
        return Math.sqrt(this.lengthSquared());
    }
    public normalized(): Quaterniond {
        const l = this.length();
        return Math.abs(l) < VSDK.EPSILON ? this : new Quaterniond(this.dv.multiply(1 / l), this.mv * (1 / l));
    }
    public conjugated(): Quaterniond {
        return new Quaterniond(this.dv.multiply(-1), this.mv);
    }
    public rotate(v: Vector3Dd): Vector3Dd {
        if (v === null) throw new TypeError("Vector to rotate cannot be null");
        const uv = this.dv.crossProduct(v);
        return v.add(uv.multiply(2 * this.mv)).add(this.dv.crossProduct(uv).multiply(2));
    }
    public withDirection(d: Vector3Dd): Quaterniond {
        return new Quaterniond(d, this.mv);
    }
    public withMagnitude(m: number): Quaterniond {
        return new Quaterniond(this.dv, m);
    }
    public direction(): Vector3Dd {
        return this.dv;
    }
    public magnitude(): number {
        return this.mv;
    }
    public epsilonEquals(o: Quaterniond | null, e = VSDK.EPSILON): boolean {
        if (o === null) return false;
        if (e < 0) throw new RangeError("epsilon must be >= 0");
        return this.dv.epsilonEquals(o.dv, e) && Math.abs(this.mv - o.mv) <= e;
    }
    public override toString(): string {
        return `${this.dv.toString()} / ${VSDK.formatDouble(this.mv)}`;
    }
}
