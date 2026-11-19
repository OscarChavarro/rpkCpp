import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Double } from "../../../../java/lang/Double.js";
export class Vector2Dd extends FundamentalEntity {
    public x: number;
    public y: number;
    public constructor();
    public constructor(x: number, y: number);
    public constructor(o: Vector2Dd);
    public constructor(a: number | Vector2Dd = 0, b = 0) {
        super();
        if (a instanceof Vector2Dd) {
            this.x = a.x;
            this.y = a.y;
        } else {
            this.x = a;
            this.y = b;
        }
    }
    public static copyOf(o: Vector2Dd): Vector2Dd {
        if (o === null) throw new TypeError("Vector2Dd to copy cannot be null");
        return new Vector2Dd(o);
    }
    public multiply(a: number): Vector2Dd {
        return new Vector2Dd(a * this.x, a * this.y);
    }
    public length(): number {
        return Math.hypot(this.x, this.y);
    }
    public static distance(a: Vector2Dd, b: Vector2Dd): number {
        return Math.hypot(a.x - b.x, a.y - b.y);
    }
    public add(o: Vector2Dd): Vector2Dd {
        return new Vector2Dd(this.x + o.x, this.y + o.y);
    }
    public set(a: Vector2Dd): void;
    public set(x: number, y: number): void;
    public set(a: number | Vector2Dd, b?: number): void {
        if (a instanceof Vector2Dd) {
            this.x = a.x;
            this.y = a.y;
        } else {
            this.x = a;
            this.y = b!;
        }
    }
    public withX(x: number): Vector2Dd {
        return new Vector2Dd(x, this.y);
    }
    public withY(y: number): Vector2Dd {
        return new Vector2Dd(this.x, y);
    }
    public epsilonEquals(o: Vector2Dd | null, e = VSDK.EPSILON): boolean {
        if (o === null) return false;
        if (e < 0) throw new RangeError("epsilon must be >= 0");
        return Math.abs(this.x - o.x) <= e && Math.abs(this.y - o.y) <= e;
    }
    public equals(other: unknown): boolean {
        return (
            other instanceof Vector2Dd && Double.compare(this.x, other.x) === 0 && Double.compare(this.y, other.y) === 0
        );
    }
    public hashCode(): number {
        return (Math.imul(31, Double.hashCode(this.x)) + Double.hashCode(this.y)) | 0;
    }
    public override toString(): string {
        return `<${VSDK.formatDouble(this.x)}, ${VSDK.formatDouble(this.y)}>`;
    }
}
