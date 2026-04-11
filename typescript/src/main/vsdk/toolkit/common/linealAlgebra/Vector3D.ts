import { CoordinateAxis } from "./CoordinateAxis";
import { Numeric } from "./Numeric";

export class Vector3D {
  public x: number;
  public y: number;
  public z: number;

  public constructor(a = 0.0, b = 0.0, c = 0.0) {
    this.x = a;
    this.y = b;
    this.z = c;
  }

  public static unitX(): Vector3D {
    return new Vector3D(1.0, 0.0, 0.0);
  }

  public static unitY(): Vector3D {
    return new Vector3D(0.0, 1.0, 0.0);
  }

  public static unitZ(): Vector3D {
    return new Vector3D(0.0, 0.0, 1.0);
  }

  public transform(xAxis: Vector3D, yAxis: Vector3D, zAxis: Vector3D): Vector3D {
    return new Vector3D(
      xAxis.dotProduct(this),
      yAxis.dotProduct(this),
      zAxis.dotProduct(this)
    );
  }

  public tolerance(epsilon: number): number {
    return epsilon * (globalThis.Math.abs(this.x) + globalThis.Math.abs(this.y) + globalThis.Math.abs(this.z));
  }

  public equals(w: Vector3D, epsilon: number): boolean {
    return Numeric.doubleEqual(this.x, w.x, epsilon) &&
      Numeric.doubleEqual(this.y, w.y, epsilon) &&
      Numeric.doubleEqual(this.z, w.z, epsilon);
  }

  public dominantCoordinate(): CoordinateAxis {
    const ax = globalThis.Math.abs(this.x);
    const ay = globalThis.Math.abs(this.y);
    const az = globalThis.Math.abs(this.z);
    const indexValue = globalThis.Math.max(ax, globalThis.Math.max(ay, az));

    if (indexValue === ax) {
      return CoordinateAxis.X;
    }
    return indexValue === ay ? CoordinateAxis.Y : CoordinateAxis.Z;
  }

  public dotProduct(b: Vector3D): number {
    return this.x * b.x + this.y * b.y + this.z * b.z;
  }

  public norm2(): number {
    return this.x * this.x + this.y * this.y + this.z * this.z;
  }

  public norm(): number {
    return globalThis.Math.sqrt(this.norm2());
  }

  public distance(p2: Vector3D): number {
    const d = new Vector3D();
    d.subtraction(p2, this);
    return d.norm();
  }

  public distance2(p2: Vector3D): number {
    const d = new Vector3D();
    d.subtraction(p2, this);
    return d.norm2();
  }

  public set(xParam: number, yParam: number, zParam: number): void {
    this.x = xParam;
    this.y = yParam;
    this.z = zParam;
  }

  public copy(v: Vector3D): void {
    this.x = v.x;
    this.y = v.y;
    this.z = v.z;
  }

  public combine(a: number, v: Vector3D, b: number, w: Vector3D): void {
    this.x = a * v.x + b * w.x;
    this.y = a * v.y + b * w.y;
    this.z = a * v.z + b * w.z;
  }

  public addition(a: Vector3D, b: Vector3D): void {
    this.x = a.x + b.x;
    this.y = a.y + b.y;
    this.z = a.z + b.z;
  }

  public subtraction(a: Vector3D, b: Vector3D): void {
    this.x = a.x - b.x;
    this.y = a.y - b.y;
    this.z = a.z - b.z;
  }

  public sumScaled(a: Vector3D, s: number, b: Vector3D): void {
    this.x = a.x + s * b.x;
    this.y = a.y + s * b.y;
    this.z = a.z + s * b.z;
  }

  public scaledCopy(s: number, v: Vector3D): void {
    this.x = s * v.x;
    this.y = s * v.y;
    this.z = s * v.z;
  }

  public inverseScaledCopy(s: number, v: Vector3D, epsilon: number): void {
    const normalizedFactor = (s < -epsilon || s > epsilon) ? 1.0 / s : 1.0;
    this.x = normalizedFactor * v.x;
    this.y = normalizedFactor * v.y;
    this.z = normalizedFactor * v.z;
  }

  public normalize(epsilon: number): void {
    const n = this.norm();
    this.inverseScaledCopy(n, this, epsilon);
  }

  public normalized(): Vector3D {
    const out = new Vector3D();
    out.copy(this);
    out.normalize(Numeric.EPSILON_FLOAT);
    return out;
  }

  public crossProduct(a: Vector3D, b: Vector3D): void {
    this.x = a.y * b.z - a.z * b.y;
    this.y = a.z * b.x - a.x * b.z;
    this.z = a.x * b.y - a.y * b.x;
  }

  public combine3(o: Vector3D, a: number, v: Vector3D, b: number, w: Vector3D): void {
    this.x = o.x + a * v.x + b * w.x;
    this.y = o.y + a * v.y + b * w.y;
    this.z = o.z + a * v.z + b * w.z;
  }

  public tripleCrossProduct(v1: Vector3D, v2: Vector3D, v3: Vector3D): void {
    const d1 = new Vector3D();
    const d2 = new Vector3D();
    d1.subtraction(v3, v2);
    d2.subtraction(v1, v2);
    this.crossProduct(d1, d2);
  }

  public midPoint(p1: Vector3D, p2: Vector3D): void {
    this.x = 0.5 * (p1.x + p2.x);
    this.y = 0.5 * (p1.y + p2.y);
    this.z = 0.5 * (p1.z + p2.z);
  }

  public toString(): string {
    return `Vector3D{x=${this.x}, y=${this.y}, z=${this.z}}`;
  }
}
