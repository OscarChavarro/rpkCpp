import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Double } from "../../../../java/lang/Double.js";
export class Vector3Dd extends FundamentalEntity {
    private readonly xv: number;
    private readonly yv: number;
    private readonly zv: number;
    public constructor();
    public constructor(x: number, y: number, z: number);
    public constructor(o: Vector3Dd);
    public constructor(a: number | Vector3Dd = 0, b = 0, c = 0) {
        super();
        if (typeof a === "number") {
            this.xv = a;
            this.yv = b;
            this.zv = c;
        } else {
            this.xv = a.x();
            this.yv = a.y();
            this.zv = a.z();
        }
    }
    public static copyOf(o: Vector3Dd): Vector3Dd {
        if (o === null) throw new TypeError("Vector3Dd to copy cannot be null");
        return new Vector3Dd(o);
    }
    public multiply(a: number): Vector3Dd;
    /** Componentwise product, as cpp `Vector3Dd::multiply(const Vector3Dd&)`. */
    public multiply(other: Vector3Dd): Vector3Dd;
    public multiply(a: number | Vector3Dd): Vector3Dd {
        if (a instanceof Vector3Dd) return new Vector3Dd(this.xv * a.xv, this.yv * a.yv, this.zv * a.zv);
        return new Vector3Dd(a * this.xv, a * this.yv, a * this.zv);
    }
    /** As cpp `Vector3Dd::midpoint`. */
    public midpoint(o: Vector3Dd): Vector3Dd {
        return new Vector3Dd(0.5 * (this.xv + o.xv), 0.5 * (this.yv + o.yv), 0.5 * (this.zv + o.zv));
    }
    public crossProduct(o: Vector3Dd): Vector3Dd {
        return new Vector3Dd(
            this.yv * o.zv - this.zv * o.yv,
            this.zv * o.xv - this.xv * o.zv,
            this.xv * o.yv - this.yv * o.xv,
        );
    }
    public dotProduct(o: Vector3Dd): number {
        return this.xv * o.xv + this.yv * o.yv + this.zv * o.zv;
    }
    public normalized(): Vector3Dd {
        let t = this.dotProduct(this);
        if (t <= VSDK.EPSILON * VSDK.EPSILON) return this;
        if (t !== 1) t = 1 / Math.sqrt(t);
        return this.multiply(t);
    }
    public normalizedFast(): Vector3Dd {
        const vTemp = Math.sqrt(this.xv * this.xv + this.yv * this.yv + this.zv * this.zv);
        return new Vector3Dd(this.xv / vTemp, this.yv / vTemp, this.zv / vTemp);
    }
    public length(): number {
        return Math.sqrt(this.dotProduct(this));
    }
    public static distance(a: Vector3Dd, b: Vector3Dd): number {
        return a.subtract(b).length();
    }
    public add(o: Vector3Dd): Vector3Dd {
        return new Vector3Dd(this.xv + o.xv, this.yv + o.yv, this.zv + o.zv);
    }
    public subtract(o: Vector3Dd): Vector3Dd {
        return new Vector3Dd(this.xv - o.xv, this.yv - o.yv, this.zv - o.zv);
    }
    public exportToFloatArrayVector(): Float32Array {
        return new Float32Array([this.xv, this.yv, this.zv, 1]);
    }
    public obtainSphericalThetaAngle(): number {
        let v: number;
        if (Math.abs(this.xv) > VSDK.EPSILON)
            v = this.xv > 0 ? Math.atan(this.yv / this.xv) : Math.PI + Math.atan(this.yv / this.xv);
        else v = this.yv > 0 ? Math.PI / 2 : (3 * Math.PI) / 2;
        while (v < 0) v += 2 * Math.PI;
        while (v > 2 * Math.PI) v -= 2 * Math.PI;
        return v;
    }
    public obtainSphericalPhiAngle(): number {
        const r = this.length();
        return r < VSDK.EPSILON ? 0 : Math.acos(this.zv / r);
    }
    public static fromSpherical(r: number, t: number, p: number): Vector3Dd {
        return new Vector3Dd(r * Math.sin(p) * Math.cos(t), r * Math.sin(p) * Math.sin(t), r * Math.cos(p));
    }
    public withX(x: number): Vector3Dd {
        return new Vector3Dd(x, this.yv, this.zv);
    }
    public withY(y: number): Vector3Dd {
        return new Vector3Dd(this.xv, y, this.zv);
    }
    public withZ(z: number): Vector3Dd {
        return new Vector3Dd(this.xv, this.yv, z);
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
    public epsilonEquals(o: Vector3Dd | null, e = VSDK.EPSILON): boolean {
        if (o === null) return false;
        if (e < 0) throw new RangeError("epsilon must be >= 0");
        return Math.abs(this.xv - o.xv) <= e && Math.abs(this.yv - o.yv) <= e && Math.abs(this.zv - o.zv) <= e;
    }
    public equals(other: unknown): boolean {
        return (
            other instanceof Vector3Dd &&
            Double.compare(this.xv, other.xv) === 0 &&
            Double.compare(this.yv, other.yv) === 0 &&
            Double.compare(this.zv, other.zv) === 0
        );
    }
    public hashCode(): number {
        let result = Double.hashCode(this.xv);
        result = (Math.imul(31, result) + Double.hashCode(this.yv)) | 0;
        return (Math.imul(31, result) + Double.hashCode(this.zv)) | 0;
    }
    public override toString(): string {
        return `<${VSDK.formatDouble(this.xv)}, ${VSDK.formatDouble(this.yv)}, ${VSDK.formatDouble(this.zv)}>`;
    }
}
