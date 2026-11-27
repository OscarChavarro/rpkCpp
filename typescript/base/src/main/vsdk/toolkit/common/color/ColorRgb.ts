import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Double } from "../../../../java/lang/Double.js";
/**
Immutable RGB color. Every arithmetic operation returns a new color and
leaves this one untouched. Component expressions are evaluated in a fixed
left-to-right order so results are bit-identical across ports.
*/
export class ColorRgb extends FundamentalEntity {
    private readonly rv: number;
    private readonly gv: number;
    private readonly bv: number;
    public constructor();
    public constructor(r: number, g: number, b: number);
    public constructor(o: ColorRgb);
    public constructor(a: number | ColorRgb = 0, b = 0, c = 0) {
        super();
        if (a instanceof ColorRgb) {
            this.rv = a.rv;
            this.gv = a.gv;
            this.bv = a.bv;
        } else {
            this.rv = a;
            this.gv = b;
            this.bv = c;
        }
    }
    public getR(): number {
        return this.rv;
    }
    public getG(): number {
        return this.gv;
    }
    public getB(): number {
        return this.bv;
    }
    public r(): number {
        return this.rv;
    }
    public g(): number {
        return this.gv;
    }
    public b(): number {
        return this.bv;
    }
    public exportToFloatArrayVector(): Float32Array {
        return new Float32Array([this.rv, this.gv, this.bv, 1]);
    }
    public distance(o: ColorRgb): number {
        return ColorRgb.distance(this, o);
    }
    public static distance(a: ColorRgb, b: ColorRgb): number {
        const dr = a.rv - b.rv;
        const dg = a.gv - b.gv;
        const db = a.bv - b.bv;
        return Math.sqrt(dr * dr + dg * dg + db * db);
    }
    public add(o: ColorRgb): ColorRgb {
        return new ColorRgb(this.rv + o.rv, this.gv + o.gv, this.bv + o.bv);
    }

    /** Component-wise this + scalar * other. */
    public addScaled(scalar: number, other: ColorRgb): ColorRgb {
        return new ColorRgb(this.rv + scalar * other.rv, this.gv + scalar * other.gv, this.bv + scalar * other.bv);
    }

    /** Component-wise this - other. */
    public subtract(other: ColorRgb): ColorRgb {
        return new ColorRgb(this.rv - other.rv, this.gv - other.gv, this.bv - other.bv);
    }

    /** this * scalar, or the component-wise (Hadamard) product this * other. */
    public multiply(scalar: number): ColorRgb;
    public multiply(other: ColorRgb): ColorRgb;
    public multiply(s: number | ColorRgb): ColorRgb {
        if (s instanceof ColorRgb) {
            return new ColorRgb(this.rv * s.rv, this.gv * s.gv, this.bv * s.bv);
        }
        return new ColorRgb(this.rv * s, this.gv * s, this.bv * s);
    }

    /**
    Multiplies by the reciprocal of scalar (not a true division: the
    reciprocal is computed once). A zero scalar returns this color unchanged.
    */
    public scaleInverse(scalar: number): ColorRgb {
        const a = scalar !== 0.0 ? 1.0 / scalar : 1.0;
        return new ColorRgb(a * this.rv, a * this.gv, a * this.bv);
    }

    /**
    Component-wise this / other. A component whose divisor is zero is
    returned unchanged.
    */
    public divide(other: ColorRgb): ColorRgb {
        return new ColorRgb(
            other.rv !== 0.0 ? this.rv / other.rv : this.rv,
            other.gv !== 0.0 ? this.gv / other.gv : this.gv,
            other.bv !== 0.0 ? this.bv / other.bv : this.bv,
        );
    }

    /** Component-wise absolute value. */
    public absolute(): ColorRgb {
        return new ColorRgb(Math.abs(this.rv), Math.abs(this.gv), Math.abs(this.bv));
    }

    /** Component-wise minimum. */
    public minimum(other: ColorRgb): ColorRgb {
        return new ColorRgb(
            this.rv < other.rv ? this.rv : other.rv,
            this.gv < other.gv ? this.gv : other.gv,
            this.bv < other.bv ? this.bv : other.bv,
        );
    }

    /** Component-wise maximum. */
    public maximum(other: ColorRgb): ColorRgb {
        return new ColorRgb(
            this.rv > other.rv ? this.rv : other.rv,
            this.gv > other.gv ? this.gv : other.gv,
            this.bv > other.bv ? this.bv : other.bv,
        );
    }

    /** |r| + |g| + |b|. */
    public sumAbsComponents(): number {
        return Math.abs(this.rv) + Math.abs(this.gv) + Math.abs(this.bv);
    }

    /** Largest of the three components. */
    public maximumComponent(): number {
        return this.rv > this.gv ? (this.rv > this.bv ? this.rv : this.bv) : this.gv > this.bv ? this.gv : this.bv;
    }

    /** (r + g + b) / 3. */
    public average(): number {
        return (this.rv + this.gv + this.bv) / 3.0;
    }

    /** True when every component lies strictly inside (-VSDK.EPSILON, VSDK.EPSILON). */
    public isBlack(): boolean {
        return (
            this.rv > -VSDK.EPSILON &&
            this.rv < VSDK.EPSILON &&
            this.gv > -VSDK.EPSILON &&
            this.gv < VSDK.EPSILON &&
            this.bv > -VSDK.EPSILON &&
            this.bv < VSDK.EPSILON
        );
    }
    public equals(o: unknown): boolean {
        return (
            o instanceof ColorRgb && Object.is(this.rv, o.rv) && Object.is(this.gv, o.gv) && Object.is(this.bv, o.bv)
        );
    }
    public hashCode(): number {
        let hash = 5;
        hash = (Math.imul(59, hash) + Double.hashCode(this.rv)) | 0;
        hash = (Math.imul(59, hash) + Double.hashCode(this.gv)) | 0;
        return (Math.imul(59, hash) + Double.hashCode(this.bv)) | 0;
    }
    public override toString(): string {
        return `<${VSDK.formatDouble(this.rv)}, ${VSDK.formatDouble(this.gv)}, ${VSDK.formatDouble(this.bv)}>`;
    }
}
