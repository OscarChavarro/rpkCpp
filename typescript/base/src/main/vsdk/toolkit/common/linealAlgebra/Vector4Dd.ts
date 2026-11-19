import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Double } from "../../../../java/lang/Double.js";
import { Vector3Dd } from "./Vector3Dd.js";
export class Vector4Dd extends FundamentalEntity {
    private readonly xv: number;
    private readonly yv: number;
    private readonly zv: number;
    private readonly wv: number;
    public constructor(x: number, y: number, z: number, w: number);
    public constructor(other: Vector4Dd | Vector3Dd);
    public constructor(a: number | Vector4Dd | Vector3Dd, b = 0, c = 0, d = 1) {
        super();
        if (typeof a === "number") {
            this.xv = a;
            this.yv = b;
            this.zv = c;
            this.wv = d;
        } else if (a instanceof Vector4Dd) {
            this.xv = a.x();
            this.yv = a.y();
            this.zv = a.z();
            this.wv = a.w();
        } else {
            this.xv = a.x();
            this.yv = a.y();
            this.zv = a.z();
            this.wv = 1;
        }
    }
    public multiply(a: number): Vector4Dd {
        return new Vector4Dd(a * this.xv, a * this.yv, a * this.zv, a * this.wv);
    }
    public dividedByW(): Vector4Dd {
        return Math.abs(this.wv) < VSDK.EPSILON
            ? this
            : new Vector4Dd(this.xv / this.wv, this.yv / this.wv, this.zv / this.wv, 1);
    }
    public length(): number {
        return Math.hypot(this.xv, this.yv, this.zv, this.wv);
    }
    public add(o: Vector4Dd): Vector4Dd {
        return new Vector4Dd(this.xv + o.xv, this.yv + o.yv, this.zv + o.zv, this.wv + o.wv);
    }
    public withX(x: number): Vector4Dd {
        return new Vector4Dd(x, this.yv, this.zv, this.wv);
    }
    public withY(y: number): Vector4Dd {
        return new Vector4Dd(this.xv, y, this.zv, this.wv);
    }
    public withZ(z: number): Vector4Dd {
        return new Vector4Dd(this.xv, this.yv, z, this.wv);
    }
    public withW(w: number): Vector4Dd {
        return new Vector4Dd(this.xv, this.yv, this.zv, w);
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
    public epsilonEquals(o: Vector4Dd | null, e = VSDK.EPSILON): boolean {
        if (o === null) return false;
        if (e < 0) throw new RangeError("epsilon must be >= 0");
        return (
            Math.abs(this.xv - o.xv) <= e &&
            Math.abs(this.yv - o.yv) <= e &&
            Math.abs(this.zv - o.zv) <= e &&
            Math.abs(this.wv - o.wv) <= e
        );
    }
    public equals(other: unknown): boolean {
        return (
            other instanceof Vector4Dd &&
            Double.compare(this.xv, other.xv) === 0 &&
            Double.compare(this.yv, other.yv) === 0 &&
            Double.compare(this.zv, other.zv) === 0 &&
            Double.compare(this.wv, other.wv) === 0
        );
    }
    public hashCode(): number {
        let result = Double.hashCode(this.xv);
        result = (Math.imul(31, result) + Double.hashCode(this.yv)) | 0;
        result = (Math.imul(31, result) + Double.hashCode(this.zv)) | 0;
        return (Math.imul(31, result) + Double.hashCode(this.wv)) | 0;
    }
    public override toString(): string {
        return `<${VSDK.formatDouble(this.xv)}, ${VSDK.formatDouble(this.yv)}, ${VSDK.formatDouble(this.zv)}, ${VSDK.formatDouble(this.wv)}>`;
    }
}
