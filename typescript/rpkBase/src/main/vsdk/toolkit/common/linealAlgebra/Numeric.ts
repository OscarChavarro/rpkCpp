import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";

export class Numeric {
    public static readonly HUGE_DOUBLE_VALUE = 1e30;
    public static readonly EPSILON = 1e-6;

    private constructor() {}

    public static doubleEqual(a: number, b: number, tolerance: number): boolean {
        return a - b > -tolerance && a - b < tolerance;
    }

    /**
    Two vectors are equal if their components are equal within the given
    tolerance, compared in double precision as doubleEqual does
    */
    public static vectorsEqual(a: Vector3Dd, b: Vector3Dd, tolerance: number): boolean {
        return (
            Numeric.doubleEqual(a.x(), b.x(), tolerance) &&
            Numeric.doubleEqual(a.y(), b.y(), tolerance) &&
            Numeric.doubleEqual(a.z(), b.z(), tolerance)
        );
    }

    public static floatCompare(x: number, y: number): boolean {
        return x > y;
    }

    public static roundDeltaToZero(x: number[] | null, epsilon: number): void {
        if (x === null || x.length === 0) {
            return;
        }
        const value = x[0];
        if (value === undefined) {
            return;
        }
        if (value <= epsilon && value >= -epsilon) {
            x[0] = 0.0;
        }
    }
}
