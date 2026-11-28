import { IllegalArgumentException } from "../../../../java/lang/IllegalArgumentException.js";
import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Vector2Dd } from "./Vector2Dd.js";
const f = Math.fround;
export class Vector2Df extends FundamentalEntity {
    private readonly xv: number;
    private readonly yv: number;
    public constructor();
    public constructor(x: number, y: number);
    public constructor(other: Vector2Df | Vector2Dd);
    public constructor(a: number | Vector2Df | Vector2Dd = 0, b = 0) {
        super();
        if (typeof a === "number") {
            this.xv = f(a);
            this.yv = f(b);
        } else if (a instanceof Vector2Df) {
            this.xv = a.x();
            this.yv = a.y();
        } else {
            this.xv = f(a.x);
            this.yv = f(a.y);
        }
    }
    public multiply(a: number): Vector2Df {
        const af = f(a);
        return new Vector2Df(f(af * this.xv), f(af * this.yv));
    }
    public length(): number {
        return f(Math.sqrt(f(f(this.xv * this.xv) + f(this.yv * this.yv))));
    }
    public add(b: Vector2Df): Vector2Df {
        return new Vector2Df(f(this.xv + b.xv), f(this.yv + b.yv));
    }
    public subtract(b: Vector2Df): Vector2Df {
        return new Vector2Df(f(this.xv - b.xv), f(this.yv - b.yv));
    }
    /**
    Square of the length (the dot product with itself)
    */
    public lengthSquared(): number {
        return f(f(this.xv * this.xv) + f(this.yv * this.yv));
    }
    public withX(x: number): Vector2Df {
        return new Vector2Df(x, this.yv);
    }
    public withY(y: number): Vector2Df {
        return new Vector2Df(this.xv, y);
    }
    public x(): number {
        return this.xv;
    }
    public y(): number {
        return this.yv;
    }
    public epsilonEquals(o: Vector2Df | null, e = f(VSDK.EPSILON)): boolean {
        if (o === null) return false;
        e = f(e);
        if (e < 0) throw new IllegalArgumentException("epsilon must be >= 0");
        return Math.abs(f(this.xv - o.xv)) <= e && Math.abs(f(this.yv - o.yv)) <= e;
    }
}
