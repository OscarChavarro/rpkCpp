import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { Vector3Dd } from "./Vector3Dd.js";
import { Vector4Dd } from "./Vector4Dd.js";
import { Quaterniond } from "./Quaterniond.js";
import { Double } from "../../../../java/lang/Double.js";
export class Matrix4x4d extends FundamentalEntity {
    private readonly m: number[][];
    public constructor();
    public constructor(o: Matrix4x4d | number[][]);
    public constructor(o: Matrix4x4d | number[][] = Matrix4x4d.unit()) {
        super();
        const a = o instanceof Matrix4x4d ? o.m : o;
        if (a.length !== 4 || a.some((r) => r.length !== 4))
            throw new RangeError("Matrix must have 4 rows and 4 columns");
        this.m = a.map((r) => r.slice());
    }
    private static unit(): number[][] {
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
        this.pos(r, c);
        return this.m[r]![c]!;
    }
    public withVal(r: number, c: number, v: number): Matrix4x4d {
        this.pos(r, c);
        const a = this.toArrayCopy();
        a[r]![c] = v;
        return new Matrix4x4d(a);
    }
    public toArrayCopy(): number[][] {
        return this.m.map((r) => r.slice());
    }
    public withoutTranslation(): Matrix4x4d {
        const a = this.withVal(0, 3, 0).withVal(1, 3, 0).withVal(2, 3, 0);
        return a.withVal(3, 0, 0).withVal(3, 1, 0).withVal(3, 2, 0).withVal(3, 3, 1);
    }
    public extractTranslation(): Vector3Dd {
        return new Vector3Dd(this.get(0, 3), this.get(1, 3), this.get(2, 3));
    }
    public withTranslation(t: Vector3Dd): Matrix4x4d {
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
            const a = Matrix4x4d.unit().map((r) => r.map(() => 0));
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
        for (let row = 0; row < 4; row++)
            for (let column = 0; column < 4; column++) {
                const minor: number[] = [];
                for (let i = 0; i < 4; i++)
                    for (let j = 0; j < 4; j++) if (i !== row && j !== column) minor.push(this.m[i]![j]!);
                const determinant =
                    minor[0]! * minor[4]! * minor[8]! +
                    minor[3]! * minor[7]! * minor[2]! +
                    minor[6]! * minor[1]! * minor[5]! -
                    minor[2]! * minor[4]! * minor[6]! -
                    minor[5]! * minor[7]! * minor[0]! -
                    minor[8]! * minor[1]! * minor[3]!;
                result[row]![column] = (row + column) % 2 === 0 ? determinant : -determinant;
            }
        return new Matrix4x4d(result);
    }
    public determinant(): number {
        const cofactors = this.cofactors();
        let result = 0;
        for (let column = 0; column < 4; column++) result += cofactors.m[0]![column]! * this.m[0]![column]!;
        return result;
    }
    public invert(): Matrix4x4d {
        return this.cofactors()
            .transpose()
            .multiply(1 / this.determinant());
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
        if (e < 0) throw new RangeError("epsilon must be >= 0");
        return this.m.every((r, i) => r.every((v, j) => Math.abs(v - o.m[i]![j]!) <= e));
    }
    private pos(r: number, c: number): void {
        if (r < 0 || r > 3 || c < 0 || c > 3) throw new RangeError(`Matrix position out of bounds: (${r}, ${c})`);
    }
}
