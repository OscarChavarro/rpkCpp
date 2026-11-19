import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Double } from "../../../../java/lang/Double.js";
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
        return Math.hypot(a.rv - b.rv, a.gv - b.gv, a.bv - b.bv);
    }
    public add(o: ColorRgb): ColorRgb {
        return new ColorRgb(this.rv + o.rv, this.gv + o.gv, this.bv + o.bv);
    }
    public multiply(s: number): ColorRgb {
        return new ColorRgb(this.rv * s, this.gv * s, this.bv * s);
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
