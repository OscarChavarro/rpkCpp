import { FundamentalEntity } from "../FundamentalEntity.js";
import { Vector2Df } from "./Vector2Df.js";
const f = Math.fround;
/**
Single precision affine transform of the plane: a 2x2 block M and a
translation t, so that p' = M p + t:

|x'|   |m00  m01|   | x |   |t0|
|  | = |        | * |   | + |  |
|y'|   |m10  m11|   | y |   |t1|

Immutable; every operation computes in float
*/
export class Matrix2x2 extends FundamentalEntity {
    private readonly m: number[][];
    private readonly t: number[];
    /**
    The identity: M = I, t = 0
    */
    public constructor();
    public constructor(m00: number, m01: number, m10: number, m11: number, t0: number, t1: number);
    public constructor(m00 = 1, m01 = 0, m10 = 0, m11 = 1, t0 = 0, t1 = 0) {
        super();
        this.m = [
            [f(m00), f(m01)],
            [f(m10), f(m11)],
        ];
        this.t = [f(t0), f(t1)];
    }
    public get(row: number, col: number): number {
        return this.m[row]![col]!;
    }
    public getTranslation(row: number): number {
        return this.t[row]!;
    }
    /**
    M p + t
    */
    public transformPoint(p: Vector2Df): Vector2Df {
        return new Vector2Df(
            f(f(f(this.get(0, 0) * p.x()) + f(this.get(0, 1) * p.y())) + this.getTranslation(0)),
            f(f(f(this.get(1, 0) * p.x()) + f(this.get(1, 1) * p.y())) + this.getTranslation(1)),
        );
    }
    /**
    The composition this(other(p)): M = this.M * other.M and
    t = this.M * other.t + this.t
    */
    public multiply(other: Matrix2x2): Matrix2x2 {
        const m00 = this.get(0, 0);
        const m01 = this.get(0, 1);
        const m10 = this.get(1, 0);
        const m11 = this.get(1, 1);
        return new Matrix2x2(
            f(f(m00 * other.get(0, 0)) + f(m01 * other.get(1, 0))),
            f(f(m00 * other.get(0, 1)) + f(m01 * other.get(1, 1))),
            f(f(m10 * other.get(0, 0)) + f(m11 * other.get(1, 0))),
            f(f(m10 * other.get(0, 1)) + f(m11 * other.get(1, 1))),
            f(f(f(m00 * other.getTranslation(0)) + f(m01 * other.getTranslation(1))) + this.getTranslation(0)),
            f(f(f(m10 * other.getTranslation(0)) + f(m11 * other.getTranslation(1))) + this.getTranslation(1)),
        );
    }
    public equals(other: unknown): boolean {
        return (
            other instanceof Matrix2x2 &&
            this.get(0, 0) === other.get(0, 0) &&
            this.get(0, 1) === other.get(0, 1) &&
            this.get(1, 0) === other.get(1, 0) &&
            this.get(1, 1) === other.get(1, 1) &&
            this.getTranslation(0) === other.getTranslation(0) &&
            this.getTranslation(1) === other.getTranslation(1)
        );
    }
}
