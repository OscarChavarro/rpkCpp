import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Vector3Dd } from "./Vector3Dd.js";
import { Vector4Dd } from "./Vector4Dd.js";
import { Quaterniond } from "./Quaterniond.js";
import { Double } from "../../../../java/lang/Double.js";
import { IllegalArgumentException } from "../../../../java/lang/IllegalArgumentException.js";
import { IndexOutOfBoundsException } from "../../../../java/lang/IndexOutOfBoundsException.js";
import { NullPointerException } from "../../../../java/lang/NullPointerException.js";
export class Matrix4x4d extends FundamentalEntity {
    private readonly m: number[][];
    public constructor();
    public constructor(o: Matrix4x4d | number[][]);
    public constructor(o: Matrix4x4d | number[][] = Matrix4x4d.buildIdentityValues()) {
        super();
        const a = o instanceof Matrix4x4d ? o.m : o;
        Matrix4x4d.validate4x4(a);
        this.m = Matrix4x4d.deepCopy(a);
    }
    private static buildIdentityValues(): number[][] {
        return [
            [1, 0, 0, 0],
            [0, 1, 0, 0],
            [0, 0, 1, 0],
            [0, 0, 0, 1],
        ];
    }
    public static copyOf(o: Matrix4x4d | number[][]): Matrix4x4d {
        return new Matrix4x4d(o);
    }
    public static identityMatrix(): Matrix4x4d {
        return new Matrix4x4d();
    }
    public identity(): Matrix4x4d {
        return new Matrix4x4d();
    }
    public get(r: number, c: number): number {
        Matrix4x4d.validatePosition(r, c);
        return this.m[r]![c]!;
    }
    public withVal(r: number, c: number, v: number): Matrix4x4d {
        Matrix4x4d.validatePosition(r, c);
        const a = this.toArrayCopy();
        a[r]![c] = v;
        return new Matrix4x4d(a);
    }
    public toArrayCopy(): number[][] {
        return Matrix4x4d.deepCopy(this.m);
    }
    public withoutTranslation(): Matrix4x4d {
        const a = this.withVal(0, 3, 0).withVal(1, 3, 0).withVal(2, 3, 0);
        return a.withVal(3, 0, 0).withVal(3, 1, 0).withVal(3, 2, 0).withVal(3, 3, 1);
    }
    public extractTranslation(): Vector3Dd {
        return new Vector3Dd(this.get(0, 3), this.get(1, 3), this.get(2, 3));
    }
    public withTranslation(t: Vector3Dd): Matrix4x4d {
        if (t === null) throw new NullPointerException("Translation vector cannot be null");
        return this.withVal(0, 3, t.x()).withVal(1, 3, t.y()).withVal(2, 3, t.z());
    }
    public orthogonalProjection(
        left: number,
        right: number,
        down: number,
        up: number,
        near: number,
        far: number,
    ): Matrix4x4d {
        const tx = -((right + left) / (right - left));
        const ty = -((up + down) / (up - down));
        const tz = -((far + near) / (far - near));
        return new Matrix4x4d([
            [2 / (right - left), 0, 0, tx],
            [0, 2 / (up - down), 0, ty],
            [0, 0, -2 / (far - near), tz],
            [0, 0, 0, 1],
        ]);
    }
    public canonicalPerspectiveProjection(): Matrix4x4d {
        return new Matrix4x4d([
            [1, 0, 0, 0],
            [0, 1, 0, 0],
            [0, 0, 0, 0],
            [0, 0, -1, 1],
        ]);
    }
    public frustumProjection(
        left: number,
        right: number,
        down: number,
        up: number,
        near: number,
        far: number,
    ): Matrix4x4d {
        const a = (right + left) / (right - left),
            b = (up + down) / (up - down);
        const c = -((far + near) / (far - near)),
            d = -((2 * far * near) / (far - near));
        return new Matrix4x4d([
            [(2 * near) / (right - left), 0, a, 0],
            [0, (2 * near) / (up - down), b, 0],
            [0, 0, c, d],
            [0, 0, -1, 0],
        ]);
    }
    public translation(x: number | Vector3Dd, y?: number, z?: number): Matrix4x4d {
        if (x instanceof Vector3Dd) return this.translation(x.x(), x.y(), x.z());
        return new Matrix4x4d([
            [1, 0, 0, x],
            [0, 1, 0, y!],
            [0, 0, 1, z!],
            [0, 0, 0, 1],
        ]);
    }
    public scale(x: number | Vector3Dd, y?: number, z?: number): Matrix4x4d {
        if (x instanceof Vector3Dd) return this.scale(x.x(), x.y(), x.z());
        return new Matrix4x4d([
            [x, 0, 0, 0],
            [0, y!, 0, 0],
            [0, 0, z!, 0],
            [0, 0, 0, 1],
        ]);
    }
    public axisRotation(angle: number, axis: Vector3Dd): Matrix4x4d;
    public axisRotation(angle: number, x: number, y: number, z: number): Matrix4x4d;
    public axisRotation(a: number, x: number | Vector3Dd, y?: number, z?: number): Matrix4x4d {
        let X: number, Y: number, Z: number;
        if (x instanceof Vector3Dd) {
            X = x.x();
            Y = x.y();
            Z = x.z();
        } else {
            X = x;
            Y = y!;
            Z = z!;
        }
        const s = Math.sin(a);
        const c = Math.cos(a);

        const mag = Math.sqrt(X * X + Y * Y + Z * Z);
        if (mag === 0.0) {
            return this.identity();
        }

        X /= mag;
        Y /= mag;
        Z /= mag;

        const xx = X * X;
        const yy = Y * Y;
        const zz = Z * Z;
        const xy = X * Y;
        const yz = Y * Z;
        const zx = Z * X;
        const xs = X * s;
        const ys = Y * s;
        const zs = Z * s;
        const oneC = 1 - c;

        return new Matrix4x4d([
            [oneC * xx + c, oneC * xy - zs, oneC * zx + ys, 0],
            [oneC * xy + zs, oneC * yy + c, oneC * yz - xs, 0],
            [oneC * zx - ys, oneC * yz + xs, oneC * zz + c, 0],
            [0, 0, 0, 1],
        ]);
    }
    public eulerAnglesRotation(yaw: number, pitch: number, roll: number): Matrix4x4d {
        const r1 = new Matrix4x4d().axisRotation(roll, 1, 0, 0);
        const r2 = new Matrix4x4d().axisRotation(pitch, 0, -1, 0);
        const r3 = new Matrix4x4d().axisRotation(yaw, 0, 0, 1);
        return r3.multiply(r2.multiply(r1));
    }
    public multiply(o: number): Matrix4x4d;
    public multiply(o: Matrix4x4d): Matrix4x4d;
    public multiply(o: Vector3Dd): Vector3Dd;
    public multiply(o: Vector4Dd): Vector4Dd;
    public multiply(o: number | Matrix4x4d | Vector3Dd | Vector4Dd): Matrix4x4d | Vector3Dd | Vector4Dd {
        if (typeof o === "number") return new Matrix4x4d(this.m.map((r) => r.map((v) => v * o)));
        if (o instanceof Matrix4x4d) {
            const a = Array.from({ length: 4 }, () => [0, 0, 0, 0]);
            for (let i = 0; i < 4; i++)
                for (let j = 0; j < 4; j++) for (let k = 0; k < 4; k++) a[i]![j]! += this.m[i]![k]! * o.m[k]![j]!;
            return new Matrix4x4d(a);
        }
        const x = o.x(),
            y = o.y(),
            z = o.z(),
            w = o instanceof Vector4Dd ? o.w() : 1;
        const v = [0, 0, 0, 0].map(
            (_, i) => this.m[i]![0]! * x + this.m[i]![1]! * y + this.m[i]![2]! * z + this.m[i]![3]! * w,
        );
        return o instanceof Vector4Dd ? new Vector4Dd(v[0]!, v[1]!, v[2]!, v[3]!) : new Vector3Dd(v[0]!, v[1]!, v[2]!);
    }
    public transpose(): Matrix4x4d {
        return new Matrix4x4d(this.m[0]!.map((_, j) => this.m.map((r) => r[j]!)));
    }
    public cofactors(): Matrix4x4d {
        const result = Array.from({ length: 4 }, () => new Array<number>(4));
        const minor = new Array<number>(9);
        for (let row = 0; row < 4; row++) {
            for (let column = 0; column < 4; column++) {
                this.fillMinor(minor, row, column);
                const sign = (row + column) % 2 === 0 ? 1 : -1;
                result[row]![column] = sign * Matrix4x4d.determinant3x3(minor);
            }
        }
        return new Matrix4x4d(result);
    }
    public determinant(): number {
        const minor = new Array<number>(9);
        let result = 0;
        for (let column = 0, sign = 1; column < 4; column++, sign *= -1) {
            this.fillMinor(minor, 0, column);
            result += sign * Matrix4x4d.determinant3x3(minor) * this.m[0]![column]!;
        }
        return result;
    }
    public invert(): Matrix4x4d {
        const factor = 1 / this.determinant();
        return this.cofactors().transpose().multiply(factor);
    }
    public inverse(): Matrix4x4d {
        return this.invert();
    }
    public exportToQuaternion(): Quaterniond {
        const tr = this.m[0]![0]! + this.m[1]![1]! + this.m[2]![2]!;
        const q = [0, 0, 0, 0];
        let qx: number;
        let qy: number;
        let qz: number;
        let qw: number;
        const nxt = [1, 2, 0];

        if (tr > 0.0) {
            let s = Math.sqrt(tr + 1.0);
            qw = s / 2.0;
            s = 0.5 / s;
            qx = (this.m[2]![1]! - this.m[1]![2]!) * s;
            qy = (this.m[0]![2]! - this.m[2]![0]!) * s;
            qz = (this.m[1]![0]! - this.m[0]![1]!) * s;
        } else {
            let i = 0;
            if (this.m[1]![1]! > this.m[0]![0]!) i = 1;
            if (this.m[2]![2]! > this.m[i]![i]!) i = 2;
            const j = nxt[i]!;
            const k = nxt[j]!;

            let s = this.m[i]![i]! - (this.m[j]![j]! + this.m[k]![k]!) + 1.0;
            s = Math.sqrt(s);
            q[i] = s * 0.5;
            if (s !== 0.0) s = 0.5 / s;

            q[3] = (this.m[k]![j]! - this.m[j]![k]!) * s;
            q[j] = (this.m[j]![i]! + this.m[i]![j]!) * s;
            q[k] = (this.m[k]![i]! + this.m[i]![k]!) * s;

            qx = q[0]!;
            qy = q[1]!;
            qz = q[2]!;
            qw = q[3]!;
        }

        return new Quaterniond(new Vector3Dd(qx, qy, qz), qw);
    }
    public importFromQuaternion(a: Quaterniond): Matrix4x4d {
        const x2 = a.direction().x() + a.direction().x();
        const y2 = a.direction().y() + a.direction().y();
        const z2 = a.direction().z() + a.direction().z();
        const xx = a.direction().x() * x2;
        const xy = a.direction().x() * y2;
        const xz = a.direction().x() * z2;
        const yy = a.direction().y() * y2;
        const yz = a.direction().y() * z2;
        const zz = a.direction().z() * z2;
        const sx = a.magnitude() * x2;
        const sy = a.magnitude() * y2;
        const sz = a.magnitude() * z2;

        return new Matrix4x4d([
            [1 - (yy + zz), xy - sz, xz + sy, 0],
            [xy + sz, 1 - (xx + zz), yz - sx, 0],
            [xz - sy, yz + sx, 1 - (xx + yy), 0],
            [0, 0, 0, 1],
        ]);
    }
    public obtainEulerYawAngle(): number {
        const pitch = this.obtainEulerPitchAngle(),
            epsilon = 0.0004;
        let direction = this.multiply(new Vector3Dd(1, 0, 0)).withZ(0) as Vector3Dd;
        if (Math.abs(Math.PI / 2 - pitch) < epsilon) direction = this.multiply(new Vector3Dd(0, 0, -1)) as Vector3Dd;
        if (Math.abs(-Math.PI / 2 - pitch) < epsilon) direction = this.multiply(new Vector3Dd(0, 0, 1)) as Vector3Dd;
        direction = direction.normalized();
        return direction.y() <= 0 ? Math.asin(direction.x()) - Math.PI / 2 : Math.PI / 2 - Math.asin(direction.x());
    }
    public obtainEulerPitchAngle(): number {
        return Math.PI / 2 - Math.acos((this.multiply(new Vector3Dd(1, 0, 0)) as Vector3Dd).normalized().z());
    }
    public obtainEulerRollAngle(): number {
        const pitch = this.obtainEulerPitchAngle(),
            yaw = this.obtainEulerYawAngle();
        const r3 = new Matrix4x4d().axisRotation(yaw, 0, 0, 1).invert();
        const r2 = new Matrix4x4d().axisRotation(pitch, 0, -1, 0).invert();
        let r1 = r2.multiply(r3.multiply(this));
        r1 = r1.importFromQuaternion(r1.exportToQuaternion().normalized());
        return r1.get(2, 1) >= 0 ? Math.acos(r1.get(1, 1)) : -Math.acos(r1.get(1, 1));
    }
    public exportToDoubleArrayRowOrder(): Float64Array {
        return new Float64Array(this.m.flat());
    }
    public exportToFloatArrayRowOrder(): Float32Array {
        return new Float32Array(this.m.flat());
    }
    public exportToDoubleArrayColumnOrder(): Float64Array {
        return new Float64Array(this.m[0]!.map((_, column) => this.m.map((row) => row[column]!)).flat());
    }
    public exportToFloatArrayColumnOrder(): Float32Array {
        return new Float32Array(this.exportToDoubleArrayColumnOrder());
    }
    public equals(other: unknown): boolean {
        return (
            other instanceof Matrix4x4d &&
            this.m.every((row, i) => row.every((value, j) => Double.compare(value, other.m[i]![j]!) === 0))
        );
    }
    public hashCode(): number {
        let result = 1;
        for (const row of this.m) {
            let rowHash = 1;
            for (const value of row) rowHash = (Math.imul(31, rowHash) + Double.hashCode(value)) | 0;
            result = (Math.imul(31, result) + rowHash) | 0;
        }
        return result;
    }
    public override toString(): string {
        return `\n------------------------------\n${this.m.map((row) => `${row.map((value) => VSDK.formatDouble(value)).join(" ")} `).join("\n")}\n------------------------------\n`;
    }
    public epsilonEquals(o: Matrix4x4d | null, e = VSDK.EPSILON): boolean {
        if (o === null) return false;
        if (e < 0) throw new IllegalArgumentException("epsilon must be >= 0");
        for (let i = 0; i < 4; i++) {
            for (let j = 0; j < 4; j++) {
                if (Math.abs(this.m[i]![j]! - o.m[i]![j]!) > e) return false;
            }
        }
        return true;
    }

    private static validate4x4(values: number[][] | null): void {
        if (values === null || values.length !== 4) throw new IllegalArgumentException("Matrix must have 4 rows");
        for (let i = 0; i < 4; i++) {
            if (values[i]?.length !== 4) throw new IllegalArgumentException(`Matrix row ${i} must have 4 columns`);
        }
    }

    private static validatePosition(r: number, c: number): void {
        if (!Number.isInteger(r) || !Number.isInteger(c) || r < 0 || r >= 4 || c < 0 || c >= 4) {
            throw new IndexOutOfBoundsException(`Matrix position out of bounds: (${r}, ${c})`);
        }
    }

    private static deepCopy(values: number[][]): number[][] {
        return values.map((row) => row.slice());
    }

    private static determinant3x3(minor: number[]): number {
        return (
            minor[0]! * minor[4]! * minor[8]! +
            minor[3]! * minor[7]! * minor[2]! +
            minor[6]! * minor[1]! * minor[5]! -
            minor[2]! * minor[4]! * minor[6]! -
            minor[5]! * minor[7]! * minor[0]! -
            minor[8]! * minor[1]! * minor[3]!
        );
    }

    private fillMinor(minor: number[], rowPivot: number, columnPivot: number): void {
        let index = 0;
        for (let i = 0; i < 4; i++) {
            for (let j = 0; j < 4; j++) {
                if (i !== rowPivot && j !== columnPivot) minor[index++] = this.m[i]![j]!;
            }
        }
    }
}
