import { IllegalArgumentException } from "../../../../java/lang/IllegalArgumentException.js";
import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Vector3Df } from "./Vector3Df.js";
import { Vector4Dd } from "./Vector4Dd.js";
const f = Math.fround;
export class Vector4Df extends FundamentalEntity {
    private readonly xv: number;
    private readonly yv: number;
    private readonly zv: number;
    private readonly wv: number;
    public constructor(x: number, y: number, z: number, w: number);
    public constructor(o: Vector4Df | Vector4Dd | Vector3Df);
    public constructor(a: number | Vector4Df | Vector4Dd | Vector3Df, b = 0, c = 0, d = 1) {
        super();
        if (typeof a === "number") {
            this.xv = f(a);
            this.yv = f(b);
            this.zv = f(c);
            this.wv = f(d);
        } else if (a instanceof Vector3Df) {
            this.xv = a.x();
            this.yv = a.y();
            this.zv = a.z();
            this.wv = 1;
        } else {
            this.xv = f(a.x());
            this.yv = f(a.y());
            this.zv = f(a.z());
            this.wv = f(a.w());
        }
    }
    public multiply(a: number): Vector4Df {
        const af = f(a);
        return new Vector4Df(f(af * this.xv), f(af * this.yv), f(af * this.zv), f(af * this.wv));
    }
    public dividedByW(): Vector4Df {
        return Math.abs(this.wv) < f(VSDK.EPSILON)
            ? this
            : new Vector4Df(f(this.xv / this.wv), f(this.yv / this.wv), f(this.zv / this.wv), 1);
    }
    public length(): number {
        const { xv, yv, zv, wv } = this;
        const xy = f(f(xv * xv) + f(yv * yv));
        return f(Math.sqrt(f(f(xy + f(zv * zv)) + f(wv * wv))));
    }
    public add(o: Vector4Df): Vector4Df {
        return new Vector4Df(f(this.xv + o.xv), f(this.yv + o.yv), f(this.zv + o.zv), f(this.wv + o.wv));
    }
    public withX(x: number): Vector4Df {
        return new Vector4Df(x, this.yv, this.zv, this.wv);
    }
    public withY(y: number): Vector4Df {
        return new Vector4Df(this.xv, y, this.zv, this.wv);
    }
    public withZ(z: number): Vector4Df {
        return new Vector4Df(this.xv, this.yv, z, this.wv);
    }
    public withW(w: number): Vector4Df {
        return new Vector4Df(this.xv, this.yv, this.zv, w);
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
    public w(): number {
        return this.wv;
    }
    public epsilonEquals(o: Vector4Df | null, e = f(VSDK.EPSILON)): boolean {
        if (o === null) return false;
        e = f(e);
        if (e < 0) throw new IllegalArgumentException("epsilon must be >= 0");
        return (
            Math.abs(f(this.xv - o.xv)) <= e &&
            Math.abs(f(this.yv - o.yv)) <= e &&
            Math.abs(f(this.zv - o.zv)) <= e &&
            Math.abs(f(this.wv - o.wv)) <= e
        );
    }
}
