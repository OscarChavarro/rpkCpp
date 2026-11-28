import { Double } from "../../../../java/lang/Double.js";
import { FundamentalEntity } from "../FundamentalEntity.js";
import { Vector2Dd } from "./Vector2Dd.js";
import { checkArrayIndex } from "../../media/_pixel.js";
/**
Double precision affine transform of the plane: a 2x2 block M and a
translation t, so that p' = M p + t:

|x'|   |m00  m01|   | x |   |t0|
|  | = |        | * |   | + |  |
|y'|   |m10  m11|   | y |   |t1|

Immutable; every operation computes in double
*/
export class Matrix2x2 extends FundamentalEntity {
    private readonly m: number[][];
    private readonly t: number[];
    /**
    The identity: M = I, t = 0
    */
    public constructor();
    public constructor(m00: number, m01: number, m10: number, m11: number, t0: number, t1: number);
    public constructor(other: Matrix2x2);
    public constructor(a: number | Matrix2x2 = 1.0, m01 = 0.0, m10 = 0.0, m11 = 1.0, t0 = 0.0, t1 = 0.0) {
        super();
        if (a instanceof Matrix2x2) {
            const other: Matrix2x2 = a;
            this.m = [
                [other.m[0]![0]!, other.m[0]![1]!],
                [other.m[1]![0]!, other.m[1]![1]!],
            ];
            this.t = [other.t[0]!, other.t[1]!];
            return;
        }
        this.m = [
            [a, m01],
            [m10, m11],
        ];
        this.t = [t0, t1];
    }

    /** Java's `m[row][col]`, with the array index checks. */
    public get(row: number, col: number): number {
        checkArrayIndex(row, 2);
        checkArrayIndex(col, 2);
        return this.m[row]![col]!;
    }

    /** Java's `t[row]`, with the array index check. */
    public getTranslation(row: number): number {
        checkArrayIndex(row, 2);
        return this.t[row]!;
    }

    /**
    M p + t
    */
    public transformPoint(p: Vector2Dd): Vector2Dd {
        const m = this.m,
            t = this.t;
        return new Vector2Dd(m[0]![0]! * p.x + m[0]![1]! * p.y + t[0]!, m[1]![0]! * p.x + m[1]![1]! * p.y + t[1]!);
    }

    /**
    The composition this(other(p)): M = this.M * other.M and
    t = this.M * other.t + this.t
    */
    public multiply(other: Matrix2x2): Matrix2x2 {
        const m = this.m,
            t = this.t;
        return new Matrix2x2(
            m[0]![0]! * other.m[0]![0]! + m[0]![1]! * other.m[1]![0]!,
            m[0]![0]! * other.m[0]![1]! + m[0]![1]! * other.m[1]![1]!,
            m[1]![0]! * other.m[0]![0]! + m[1]![1]! * other.m[1]![0]!,
            m[1]![0]! * other.m[0]![1]! + m[1]![1]! * other.m[1]![1]!,
            m[0]![0]! * other.t[0]! + m[0]![1]! * other.t[1]! + t[0]!,
            m[1]![0]! * other.t[0]! + m[1]![1]! * other.t[1]! + t[1]!,
        );
    }
    public equals(other: unknown): boolean {
        if (this === other) return true;
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

    public hashCode(): number {
        let result = Double.hashCode(this.get(0, 0) + 0);
        result = (Math.imul(31, result) + Double.hashCode(this.get(0, 1) + 0)) | 0;
        result = (Math.imul(31, result) + Double.hashCode(this.get(1, 0) + 0)) | 0;
        result = (Math.imul(31, result) + Double.hashCode(this.get(1, 1) + 0)) | 0;
        result = (Math.imul(31, result) + Double.hashCode(this.getTranslation(0) + 0)) | 0;
        return (Math.imul(31, result) + Double.hashCode(this.getTranslation(1) + 0)) | 0;
    }
}
