import { FundamentalEntity } from "../FundamentalEntity";
import { VSDK } from "../VSDK";
import { Vector3Dd } from "./Vector3Dd";
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
    public multiply(a: number): Vector3Df {
        return new Vector3Df(f(a * this.xv), f(a * this.yv), f(a * this.zv));
    }
    public crossProduct(o: Vector3Df): Vector3Df {
        return new Vector3Df(
            f(this.yv * o.zv - this.zv * o.yv),
            f(this.zv * o.xv - this.xv * o.zv),
            f(this.xv * o.yv - this.yv * o.xv),
        );
    }
    public dotProduct(o: Vector3Df): number {
        return f(f(this.xv * o.xv) + f(this.yv * o.yv) + f(this.zv * o.zv));
    }
    public normalized(): Vector3Df {
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
    public withX(x: number): Vector3Df {
        return new Vector3Df(x, this.yv, this.zv);
    }
    public withY(y: number): Vector3Df {
        return new Vector3Df(this.xv, y, this.zv);
    }
    public withZ(z: number): Vector3Df {
        return new Vector3Df(this.xv, this.yv, z);
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
        if (e < 0) throw new RangeError("epsilon must be >= 0");
        return Math.abs(this.xv - o.xv) <= e && Math.abs(this.yv - o.yv) <= e && Math.abs(this.zv - o.zv) <= e;
    }
}
