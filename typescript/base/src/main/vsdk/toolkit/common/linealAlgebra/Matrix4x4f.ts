import { FundamentalEntity } from "../FundamentalEntity.js";
import { VSDK } from "../VSDK.js";
import { VSDKFatalException } from "../VSDKFatalException.js";
import { Logger } from "../logging/Logger.js";
import { Vector3Df } from "./Vector3Df.js";
import { Vector4Df } from "./Vector4Df.js";
import { Vector3Dd } from "./Vector3Dd.js";
import { Quaterniond } from "./Quaterniond.js";
const f = Math.fround;
/**
Single precision 4x4 matrix: the sixteen values are floats and every
operation computes in float. Sums of products run from the first term to the
last (a0 * b0 + a1 * b1 + a2 * b2 + a3 * b3), so the results do not depend on
an accumulator starting at zero
*/
export class Matrix4x4f extends FundamentalEntity {
    private readonly m: number[][];
    public constructor();
    public constructor(o: Matrix4x4f | number[][]);
    public constructor(o?: Matrix4x4f | number[][]) {
        super();
        if (o === undefined) {
            this.m = [];
            for (let i = 0; i < 4; ++i) {
                const row: number[] = [];
                for (let j = 0; j < 4; ++j) {
                    row.push(i === j ? 1 : 0);
                }
                this.m.push(row);
            }
        } else if (o instanceof Matrix4x4f) {
            this.m = o.m.map((r) => r.slice());
        } else {
            if (o.length !== 4 || o.some((r) => r.length !== 4))
                throw new RangeError("Matrix must have 4 rows and 4 columns");
            this.m = o.map((r) => r.map(f));
        }
    }
    public static copyOf(o: Matrix4x4f | number[][]): Matrix4x4f {
        return new Matrix4x4f(o);
    }
    public static identityMatrix(): Matrix4x4f {
        return new Matrix4x4f();
    }
    public identity(): Matrix4x4f {
        return new Matrix4x4f();
    }
    public get(r: number, c: number): number {
        this.pos(r, c);
        return this.m[r]![c]!;
    }
    public withVal(r: number, c: number, v: number): Matrix4x4f {
        this.pos(r, c);
        const result = new Matrix4x4f(this);
        result.m[r]![c] = f(v);
        return result;
    }
    public toArrayCopy(): number[][] {
        return this.m.map((r) => r.slice());
    }
    public withoutTranslation(): Matrix4x4f {
        const r = new Matrix4x4f(this);
        r.m[0]![3] = r.m[1]![3] = r.m[2]![3] = 0;
        r.m[3]![0] = r.m[3]![1] = r.m[3]![2] = 0;
        r.m[3]![3] = 1;
        return r;
    }
    public extractTranslation(): Vector3Df {
        return new Vector3Df(this.m[0]![3]!, this.m[1]![3]!, this.m[2]![3]!);
    }
    public withTranslation(t: Vector3Df): Matrix4x4f {
        return this.withVal(0, 3, t.x()).withVal(1, 3, t.y()).withVal(2, 3, t.z());
    }
    public orthogonalProjection(l: number, r: number, d: number, u: number, n: number, far: number): Matrix4x4f {
        l = f(l);
        r = f(r);
        d = f(d);
        u = f(u);
        n = f(n);
        far = f(far);
        return new Matrix4x4f([
            [f(2 / f(r - l)), 0, 0, -f(f(r + l) / f(r - l))],
            [0, f(2 / f(u - d)), 0, -f(f(u + d) / f(u - d))],
            [0, 0, f(-2 / f(far - n)), -f(f(far + n) / f(far - n))],
            [0, 0, 0, 1],
        ]);
    }
    public canonicalPerspectiveProjection(): Matrix4x4f {
        return new Matrix4x4f([
            [1, 0, 0, 0],
            [0, 1, 0, 0],
            [0, 0, 0, 0],
            [0, 0, -1, 1],
        ]);
    }
    public frustumProjection(l: number, r: number, d: number, u: number, n: number, far: number): Matrix4x4f {
        l = f(l);
        r = f(r);
        d = f(d);
        u = f(u);
        n = f(n);
        far = f(far);
        return new Matrix4x4f([
            [f(f(2 * n) / f(r - l)), 0, f(f(r + l) / f(r - l)), 0],
            [0, f(f(2 * n) / f(u - d)), f(f(u + d) / f(u - d)), 0],
            [0, 0, -f(f(far + n) / f(far - n)), -f(f(f(2 * far) * n) / f(far - n))],
            [0, 0, -1, 0],
        ]);
    }
    public translation(x: number | Vector3Df, y?: number, z?: number): Matrix4x4f {
        if (x instanceof Vector3Df) return this.translation(x.x(), x.y(), x.z());
        return new Matrix4x4f().withVal(0, 3, x).withVal(1, 3, y!).withVal(2, 3, z!);
    }
    public scale(x: number | Vector3Df, y?: number, z?: number): Matrix4x4f {
        if (x instanceof Vector3Df) return this.scale(x.x(), x.y(), x.z());
        return new Matrix4x4f().withVal(0, 0, x).withVal(1, 1, y!).withVal(2, 2, z!);
    }
    public eulerAnglesRotation(yaw: number, pitch: number, roll: number): Matrix4x4f {
        const r1 = new Matrix4x4f().axisRotation(roll, 1, 0, 0);
        const r2 = new Matrix4x4f().axisRotation(pitch, 0, -1, 0);
        const r3 = new Matrix4x4f().axisRotation(yaw, 0, 0, 1);
        return r3.multiply(r2.multiply(r1));
    }
    public axisRotation(a: number, v: Vector3Df): Matrix4x4f;
    public axisRotation(a: number, x: number, y: number, z: number): Matrix4x4f;
    public axisRotation(a: number, xOrAxis: number | Vector3Df, yv?: number, zv?: number): Matrix4x4f {
        if (xOrAxis instanceof Vector3Df) return this.axisRotation(a, xOrAxis.x(), xOrAxis.y(), xOrAxis.z());
        const angle = f(a);
        let x = f(xOrAxis);
        let y = f(yv!);
        let z = f(zv!);
        const norm = f(Math.sqrt(f(f(f(x * x) + f(y * y)) + f(z * z))));
        if (Math.abs(norm) < f(VSDK.EPSILON)) return new Matrix4x4f();
        x = f(x / norm);
        y = f(y / norm);
        z = f(z / norm);
        const s = f(Math.sin(angle));
        const c = f(Math.cos(angle));
        const t = f(1 - c);
        return new Matrix4x4f([
            [f(f(f(t * x) * x) + c), f(f(f(t * x) * y) - f(s * z)), f(f(f(t * x) * z) + f(s * y)), 0],
            [f(f(f(t * x) * y) + f(s * z)), f(f(f(t * y) * y) + c), f(f(f(t * y) * z) - f(s * x)), 0],
            [f(f(f(t * x) * z) - f(s * y)), f(f(f(t * y) * z) + f(s * x)), f(f(f(t * z) * z) + c), 0],
            [0, 0, 0, 1],
        ]);
    }
    public inverse(): Matrix4x4f {
        return this.invert();
    }
    public invert(): Matrix4x4f {
        const d = this.determinant();
        if (Math.abs(d) < f(VSDK.EPSILON)) {
            Logger.reportMessage(this, Logger.ERROR, "invert", "Trying to invert a matrix with zero determinant");
            throw new VSDKFatalException("Trying to invert a matrix with zero determinant");
        }
        return this.cofactors()
            .transpose()
            .multiply(f(1 / d));
    }
    private static minor3x3(sub: number[][]): number {
        const s0 = sub[0]!;
        const s1 = sub[1]!;
        const s2 = sub[2]!;
        return f(
            f(
                f(s0[0]! * f(f(s1[1]! * s2[2]!) - f(s1[2]! * s2[1]!))) -
                    f(s0[1]! * f(f(s1[0]! * s2[2]!) - f(s1[2]! * s2[0]!))),
            ) + f(s0[2]! * f(f(s1[0]! * s2[1]!) - f(s1[1]! * s2[0]!))),
        );
    }
    public cofactors(): Matrix4x4f {
        const r = new Matrix4x4f();
        for (let row = 0; row < 4; ++row) {
            for (let col = 0; col < 4; ++col) {
                const sub: number[][] = [[], [], []];
                for (let i = 0, si = 0; i < 4; ++i) {
                    if (i === row) continue;
                    for (let j = 0, sj = 0; j < 4; ++j) {
                        if (j === col) continue;
                        sub[si]![sj++] = this.m[i]![j]!;
                    }
                    si++;
                }
                const sign = (row + col) % 2 === 0 ? 1 : -1;
                r.m[row]![col] = f(sign * Matrix4x4f.minor3x3(sub));
            }
        }
        return r;
    }
    public transpose(): Matrix4x4f {
        const r = new Matrix4x4f();
        for (let i = 0; i < 4; ++i) for (let j = 0; j < 4; ++j) r.m[j]![i] = this.m[i]![j]!;
        return r;
    }
    public multiply(o: number): Matrix4x4f;
    public multiply(o: Matrix4x4f): Matrix4x4f;
    /**
    Transforms a point: the product with (x, y, z, 1), divided by the
    resulting w unless w is within VSDK.EPSILON of zero
    */
    public multiply(o: Vector3Df): Vector3Df;
    public multiply(o: Vector4Df): Vector4Df;
    public multiply(o: number | Matrix4x4f | Vector3Df | Vector4Df): Matrix4x4f | Vector3Df | Vector4Df {
        const m = this.m;
        if (typeof o === "number") {
            const a = f(o);
            const r = new Matrix4x4f(this);
            for (let i = 0; i < 4; ++i) for (let j = 0; j < 4; ++j) r.m[i]![j] = f(r.m[i]![j]! * a);
            return r;
        }
        if (o instanceof Matrix4x4f) {
            const r = new Matrix4x4f();
            for (let row = 0; row < 4; ++row) {
                for (let col = 0; col < 4; ++col) {
                    r.m[row]![col] = f(
                        f(
                            f(f(m[row]![0]! * o.m[0]![col]!) + f(m[row]![1]! * o.m[1]![col]!)) +
                                f(m[row]![2]! * o.m[2]![col]!),
                        ) + f(m[row]![3]! * o.m[3]![col]!),
                    );
                }
            }
            return r;
        }
        if (o instanceof Vector3Df) {
            const ex = o.x();
            const ey = o.y();
            const ez = o.z();

            const rx = f(f(f(f(m[0]![0]! * ex) + f(m[0]![1]! * ey)) + f(m[0]![2]! * ez)) + m[0]![3]!);
            const ry = f(f(f(f(m[1]![0]! * ex) + f(m[1]![1]! * ey)) + f(m[1]![2]! * ez)) + m[1]![3]!);
            const rz = f(f(f(f(m[2]![0]! * ex) + f(m[2]![1]! * ey)) + f(m[2]![2]! * ez)) + m[2]![3]!);
            const rw = f(f(f(f(m[3]![0]! * ex) + f(m[3]![1]! * ey)) + f(m[3]![2]! * ez)) + m[3]![3]!);

            if (Math.abs(rw) < f(VSDK.EPSILON)) {
                return new Vector3Df(rx, ry, rz);
            }
            const invW = f(1 / rw);
            return new Vector3Df(f(rx * invW), f(ry * invW), f(rz * invW));
        }
        const ex = o.x();
        const ey = o.y();
        const ez = o.z();
        const ew = o.w();
        return new Vector4Df(
            f(f(f(f(m[0]![0]! * ex) + f(m[0]![1]! * ey)) + f(m[0]![2]! * ez)) + f(m[0]![3]! * ew)),
            f(f(f(f(m[1]![0]! * ex) + f(m[1]![1]! * ey)) + f(m[1]![2]! * ez)) + f(m[1]![3]! * ew)),
            f(f(f(f(m[2]![0]! * ex) + f(m[2]![1]! * ey)) + f(m[2]![2]! * ez)) + f(m[2]![3]! * ew)),
            f(f(f(f(m[3]![0]! * ex) + f(m[3]![1]! * ey)) + f(m[3]![2]! * ez)) + f(m[3]![3]! * ew)),
        );
    }
    public determinant(): number {
        let d = 0;
        for (let col = 0; col < 4; ++col) {
            const sub: number[][] = [[], [], []];
            for (let i = 1; i < 4; ++i) {
                let sj = 0;
                for (let j = 0; j < 4; ++j) {
                    if (j === col) continue;
                    sub[i - 1]![sj++] = this.m[i]![j]!;
                }
            }
            d = f(d + f(f((col % 2 === 0 ? 1 : -1) * this.m[0]![col]!) * Matrix4x4f.minor3x3(sub)));
        }
        return d;
    }
    public override toString(): string {
        let s = "\n------------------------------\n";
        for (let i = 0; i < 4; ++i) {
            for (let j = 0; j < 4; ++j) {
                s += `${VSDK.formatDouble(this.m[i]![j]!)} `;
            }
            s += "\n";
        }
        s += "------------------------------\n";
        return s;
    }
    public exportToDoubleArrayRowOrder(): Float64Array {
        const array = new Float64Array(16);
        for (let i = 0, k = 0; i < 4; i++) for (let j = 0; j < 4; j++, k++) array[k] = this.m[i]![j]!;
        return array;
    }
    public exportToFloatArrayRowOrder(): Float32Array {
        const array = new Float32Array(16);
        for (let i = 0, k = 0; i < 4; i++) for (let j = 0; j < 4; j++, k++) array[k] = this.m[i]![j]!;
        return array;
    }
    public exportToDoubleArrayColumnOrder(): Float64Array {
        const array = new Float64Array(16);
        for (let j = 0, k = 0; j < 4; j++) for (let i = 0; i < 4; i++, k++) array[k] = this.m[i]![j]!;
        return array;
    }
    public exportToFloatArrayColumnOrder(): Float32Array {
        const array = new Float32Array(16);
        for (let j = 0, k = 0; j < 4; j++) for (let i = 0; i < 4; i++, k++) array[k] = this.m[i]![j]!;
        return array;
    }
    public exportToQuaternion(): Quaterniond {
        const m = this.m;
        const tr = f(f(m[0]![0]! + m[1]![1]!) + m[2]![2]!);
        let qx: number;
        let qy: number;
        let qz: number;
        let qw: number;
        const q = [0, 0, 0, 0];
        const nxt = [1, 2, 0];

        if (tr > 0.0) {
            let s = f(Math.sqrt(f(tr + 1.0)));
            qw = f(s / 2.0);
            s = f(0.5 / s);
            qx = f(f(m[2]![1]! - m[1]![2]!) * s);
            qy = f(f(m[0]![2]! - m[2]![0]!) * s);
            qz = f(f(m[1]![0]! - m[0]![1]!) * s);
        } else {
            let i = 0;
            if (m[1]![1]! > m[0]![0]!) i = 1;
            if (m[2]![2]! > m[i]![i]!) i = 2;
            const j = nxt[i]!;
            const k = nxt[j]!;

            let s = f(Math.sqrt(f(f(m[i]![i]! - f(m[j]![j]! + m[k]![k]!)) + 1.0)));
            q[i] = f(s * 0.5);
            if (s !== 0.0) s = f(0.5 / s);

            q[3] = f(f(m[k]![j]! - m[j]![k]!) * s);
            q[j] = f(f(m[j]![i]! + m[i]![j]!) * s);
            q[k] = f(f(m[k]![i]! + m[i]![k]!) * s);

            qx = q[0]!;
            qy = q[1]!;
            qz = q[2]!;
            qw = q[3]!;
        }

        return new Quaterniond(new Vector3Dd(qx, qy, qz), qw);
    }
    public importFromQuaternion(a: Quaterniond): Matrix4x4f {
        const dx = f(a.direction().x());
        const dy = f(a.direction().y());
        const dz = f(a.direction().z());
        const w = f(a.magnitude());
        const x2 = f(dx + dx);
        const y2 = f(dy + dy);
        const z2 = f(dz + dz);
        const xx = f(dx * x2);
        const xy = f(dx * y2);
        const xz = f(dx * z2);
        const yy = f(dy * y2);
        const yz = f(dy * z2);
        const zz = f(dz * z2);
        const sx = f(w * x2);
        const sy = f(w * y2);
        const sz = f(w * z2);

        return new Matrix4x4f([
            [f(1 - f(yy + zz)), f(xy - sz), f(xz + sy), 0],
            [f(xy + sz), f(1 - f(xx + zz)), f(yz - sx), 0],
            [f(xz - sy), f(yz + sx), f(1 - f(xx + yy)), 0],
            [0, 0, 0, 1],
        ]);
    }
    public obtainEulerYawAngle(): number {
        const pitch = this.obtainEulerPitchAngle();
        const epsilon = f(0.0004);
        const halfPi = f(Math.PI / 2.0);

        let dir = this.multiply(new Vector3Df(1, 0, 0)).withZ(0);

        if (Math.abs(f(halfPi - pitch)) < epsilon) dir = this.multiply(new Vector3Df(0, 0, -1));
        if (Math.abs(f(-halfPi - pitch)) < epsilon) dir = this.multiply(new Vector3Df(0, 0, 1));

        dir = dir.normalized();
        if (dir.y() <= 0) return f(f(Math.asin(dir.x())) - halfPi);
        return f(halfPi - f(Math.asin(dir.x())));
    }
    public obtainEulerPitchAngle(): number {
        const dir = this.multiply(new Vector3Df(1, 0, 0)).normalized();
        return f(f(Math.PI / 2.0) - f(Math.acos(dir.z())));
    }
    public obtainEulerRollAngle(): number {
        const pitch = this.obtainEulerPitchAngle();
        const yaw = this.obtainEulerYawAngle();

        const r3 = new Matrix4x4f().axisRotation(yaw, 0, 0, 1).invert();
        const r2 = new Matrix4x4f().axisRotation(pitch, 0, -1, 0).invert();
        let r1 = r2.multiply(r3.multiply(this));

        const q = r1.exportToQuaternion().normalized();
        r1 = r1.importFromQuaternion(q);

        if (r1.get(2, 1) >= 0) return f(Math.acos(r1.get(1, 1)));
        return -f(Math.acos(r1.get(1, 1)));
    }
    public epsilonEquals(o: Matrix4x4f | null, e = f(VSDK.EPSILON)): boolean {
        if (o === null) return false;
        const eps = f(e);
        if (eps < 0.0) {
            Logger.reportMessage(this, Logger.ERROR, "epsilonEquals", "epsilon must be >= 0");
            throw new VSDKFatalException("epsilon must be >= 0");
        }
        for (let i = 0; i < 4; ++i) {
            for (let j = 0; j < 4; ++j) {
                if (Math.abs(f(this.m[i]![j]! - o.m[i]![j]!)) > eps) return false;
            }
        }
        return true;
    }
    public equals(other: unknown): boolean {
        if (!(other instanceof Matrix4x4f)) return false;
        for (let i = 0; i < 4; ++i) for (let j = 0; j < 4; ++j) if (this.m[i]![j]! !== other.m[i]![j]!) return false;
        return true;
    }
    private static hashFloat(val: number): number {
        const view = new DataView(new ArrayBuffer(4));
        view.setFloat32(0, val, true);
        let h = 0;
        for (let i = 0; i < 4; ++i) {
            h = (Math.imul(h, 31) + view.getUint8(i)) >>> 0;
        }
        return h;
    }
    public hashCode(): number {
        let result = 1;
        for (let i = 0; i < 4; i++) {
            for (let j = 0; j < 4; j++) {
                result = (Math.imul(31, result) + Matrix4x4f.hashFloat(this.m[i]![j]!)) >>> 0;
            }
        }
        return result | 0;
    }
    private pos(r: number, c: number): void {
        if (r < 0 || r > 3 || c < 0 || c > 3) throw new RangeError(`Matrix position out of bounds: (${r}, ${c})`);
    }
}
