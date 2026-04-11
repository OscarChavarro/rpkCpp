import { Numeric } from "./Numeric";
import { Vector3D } from "./Vector3D";
import { Vector4D } from "./Vector4D";

export class Matrix4x4 {
  public m: number[][];

  public constructor(...values: number[]) {
    this.m = Matrix4x4.createZeroMatrix();
    if (values.length === 16) {
      this.m[0][0] = values[0];
      this.m[0][1] = values[1];
      this.m[0][2] = values[2];
      this.m[0][3] = values[3];
      this.m[1][0] = values[4];
      this.m[1][1] = values[5];
      this.m[1][2] = values[6];
      this.m[1][3] = values[7];
      this.m[2][0] = values[8];
      this.m[2][1] = values[9];
      this.m[2][2] = values[10];
      this.m[2][3] = values[11];
      this.m[3][0] = values[12];
      this.m[3][1] = values[13];
      this.m[3][2] = values[14];
      this.m[3][3] = values[15];
    }
    else {
      this.m[0][0] = 1.0;
      this.m[1][1] = 1.0;
      this.m[2][2] = 1.0;
      this.m[3][3] = 1.0;
    }
  }

  private static createZeroMatrix(): number[][] {
    return [
      [0.0, 0.0, 0.0, 0.0],
      [0.0, 0.0, 0.0, 0.0],
      [0.0, 0.0, 0.0, 0.0],
      [0.0, 0.0, 0.0, 0.0]
    ];
  }

  public static identity(): Matrix4x4 {
    return new Matrix4x4();
  }

  public set3X3Matrix(a: number, b: number, c: number, d: number, e: number, f: number, g: number, h: number, i: number): void {
    this.m[0][0] = a;
    this.m[0][1] = b;
    this.m[0][2] = c;
    this.m[1][0] = d;
    this.m[1][1] = e;
    this.m[1][2] = f;
    this.m[2][0] = g;
    this.m[2][1] = h;
    this.m[2][2] = i;
  }

  public transformPoint3D(src: Vector3D, dst: Vector3D): void {
    const sx = src.x;
    const sy = src.y;
    const sz = src.z;
    dst.x = this.m[0][0] * sx + this.m[0][1] * sy + this.m[0][2] * sz;
    dst.y = this.m[1][0] * sx + this.m[1][1] * sy + this.m[1][2] * sz;
    dst.z = this.m[2][0] * sx + this.m[2][1] * sy + this.m[2][2] * sz;
  }

  public transformPoint4D(src: Vector4D, dst: Vector4D): void {
    const sx = src.x;
    const sy = src.y;
    const sz = src.z;
    const sw = src.w;
    dst.x = this.m[0][0] * sx + this.m[0][1] * sy + this.m[0][2] * sz + this.m[0][3] * sw;
    dst.y = this.m[1][0] * sx + this.m[1][1] * sy + this.m[1][2] * sz + this.m[1][3] * sw;
    dst.z = this.m[2][0] * sx + this.m[2][1] * sy + this.m[2][2] * sz + this.m[2][3] * sw;
    dst.w = this.m[3][0] * sx + this.m[3][1] * sy + this.m[3][2] * sz + this.m[3][3] * sw;
  }

  public recoverRotationParameters(angle: number[], axis: Vector3D): void {
    if (angle === null || angle.length === 0 || axis === null) {
      throw new Error("angle and axis output parameters are required");
    }

    const c = (this.m[0][0] + this.m[1][1] + this.m[2][2] - 1.0) * 0.5;
    if (c > 1.0 - Numeric.EPSILON) {
      angle[0] = 0.0;
      axis.set(0.0, 0.0, 1.0);
    }
    else if (c < -1.0 + Numeric.EPSILON) {
      angle[0] = globalThis.Math.PI;
      axis.x = globalThis.Math.sqrt((this.m[0][0] + 1.0) * 0.5);
      axis.y = globalThis.Math.sqrt((this.m[1][1] + 1.0) * 0.5);
      axis.z = globalThis.Math.sqrt((this.m[2][2] + 1.0) * 0.5);

      if (this.m[1][0] < 0.0) {
        axis.y = -axis.y;
      }
      if (this.m[2][0] < 0.0) {
        axis.z = -axis.z;
      }
    }
    else {
      angle[0] = globalThis.Math.acos(c);
      const s = globalThis.Math.sqrt(1.0 - c * c);
      const r = 1.0 / (2.0 * s);
      axis.x = (this.m[2][1] - this.m[1][2]) * r;
      axis.y = (this.m[0][2] - this.m[2][0]) * r;
      axis.z = (this.m[1][0] - this.m[0][1]) * r;
    }
  }

  public static createTransComposeMatrix(xf2: Matrix4x4, xf1: Matrix4x4): Matrix4x4 {
    const xf = new Matrix4x4();

    xf.m[0][0] = xf2.m[0][0] * xf1.m[0][0] + xf2.m[0][1] * xf1.m[1][0] + xf2.m[0][2] * xf1.m[2][0] + xf2.m[0][3] * xf1.m[3][0];
    xf.m[0][1] = xf2.m[0][0] * xf1.m[0][1] + xf2.m[0][1] * xf1.m[1][1] + xf2.m[0][2] * xf1.m[2][1] + xf2.m[0][3] * xf1.m[3][1];
    xf.m[0][2] = xf2.m[0][0] * xf1.m[0][2] + xf2.m[0][1] * xf1.m[1][2] + xf2.m[0][2] * xf1.m[2][2] + xf2.m[0][3] * xf1.m[3][2];
    xf.m[0][3] = xf2.m[0][0] * xf1.m[0][3] + xf2.m[0][1] * xf1.m[1][3] + xf2.m[0][2] * xf1.m[2][3] + xf2.m[0][3] * xf1.m[3][3];

    xf.m[1][0] = xf2.m[1][0] * xf1.m[0][0] + xf2.m[1][1] * xf1.m[1][0] + xf2.m[1][2] * xf1.m[2][0] + xf2.m[1][3] * xf1.m[3][0];
    xf.m[1][1] = xf2.m[1][0] * xf1.m[0][1] + xf2.m[1][1] * xf1.m[1][1] + xf2.m[1][2] * xf1.m[2][1] + xf2.m[1][3] * xf1.m[3][1];
    xf.m[1][2] = xf2.m[1][0] * xf1.m[0][2] + xf2.m[1][1] * xf1.m[1][2] + xf2.m[1][2] * xf1.m[2][2] + xf2.m[1][3] * xf1.m[3][2];
    xf.m[1][3] = xf2.m[1][0] * xf1.m[0][3] + xf2.m[1][1] * xf1.m[1][3] + xf2.m[1][2] * xf1.m[2][3] + xf2.m[1][3] * xf1.m[3][3];

    xf.m[2][0] = xf2.m[2][0] * xf1.m[0][0] + xf2.m[2][1] * xf1.m[1][0] + xf2.m[2][2] * xf1.m[2][0] + xf2.m[2][3] * xf1.m[3][0];
    xf.m[2][1] = xf2.m[2][0] * xf1.m[0][1] + xf2.m[2][1] * xf1.m[1][1] + xf2.m[2][2] * xf1.m[2][1] + xf2.m[2][3] * xf1.m[3][1];
    xf.m[2][2] = xf2.m[2][0] * xf1.m[0][2] + xf2.m[2][1] * xf1.m[1][2] + xf2.m[2][2] * xf1.m[2][2] + xf2.m[2][3] * xf1.m[3][2];
    xf.m[2][3] = xf2.m[2][0] * xf1.m[0][3] + xf2.m[2][1] * xf1.m[1][3] + xf2.m[2][2] * xf1.m[2][3] + xf2.m[2][3] * xf1.m[3][3];

    xf.m[3][0] = xf2.m[3][0] * xf1.m[0][0] + xf2.m[3][1] * xf1.m[1][0] + xf2.m[3][2] * xf1.m[2][0] + xf2.m[3][3] * xf1.m[3][0];
    xf.m[3][1] = xf2.m[3][0] * xf1.m[0][1] + xf2.m[3][1] * xf1.m[1][1] + xf2.m[3][2] * xf1.m[2][1] + xf2.m[3][3] * xf1.m[3][1];
    xf.m[3][2] = xf2.m[3][0] * xf1.m[0][2] + xf2.m[3][1] * xf1.m[1][2] + xf2.m[3][2] * xf1.m[2][2] + xf2.m[3][3] * xf1.m[3][2];
    xf.m[3][3] = xf2.m[3][0] * xf1.m[0][3] + xf2.m[3][1] * xf1.m[1][3] + xf2.m[3][2] * xf1.m[2][3] + xf2.m[3][3] * xf1.m[3][3];

    return xf;
  }

  public static createTranslationMatrix(translation: Vector3D): Matrix4x4 {
    const xf = new Matrix4x4();
    xf.m[0][3] = translation.x;
    xf.m[1][3] = translation.y;
    xf.m[2][3] = translation.z;
    return xf;
  }

  public static createPerspectiveMatrix(fieldOfViewInRadians: number, aspect: number, near: number, far: number): Matrix4x4 {
    const xf = new Matrix4x4();
    const f = 1.0 / globalThis.Math.tan(fieldOfViewInRadians / 2.0);

    xf.m[0][0] = f / aspect;
    xf.m[1][1] = f;
    xf.m[2][2] = (near + far) / (near - far);
    xf.m[2][3] = (2 * far * near) / (near - far);
    xf.m[3][2] = -1.0;
    xf.m[3][3] = 0.0;

    return xf;
  }

  public static createRotationMatrix(angleInRadians: number, axis: Vector3D): Matrix4x4 {
    const xf = new Matrix4x4();

    let s = axis.norm();
    if (s < Numeric.EPSILON) {
      return xf;
    }

    axis.inverseScaledCopy(s, axis, Numeric.EPSILON_FLOAT);

    const x = axis.x;
    const y = axis.y;
    const z = axis.z;
    const c = globalThis.Math.cos(angleInRadians);
    s = globalThis.Math.sin(angleInRadians);
    const t = 1.0 - c;

    xf.set3X3Matrix(
      x * x * t + c, x * y * t - z * s, x * z * t + y * s,
      x * y * t + z * s, y * y * t + c, y * z * t - x * s,
      x * z * t - y * s, y * z * t + x * s, z * z * t + c
    );

    return xf;
  }

  public static createLookAtMatrix(eye: Vector3D, centre: Vector3D, up: Vector3D): Matrix4x4 {
    const xf = new Matrix4x4();
    const s = new Vector3D();
    const xAxis = new Vector3D();
    const yAxis = new Vector3D();
    const zAxis = new Vector3D();

    zAxis.subtraction(eye, centre);
    zAxis.normalize(Numeric.EPSILON_FLOAT);

    xAxis.crossProduct(up, zAxis);
    xAxis.normalize(Numeric.EPSILON_FLOAT);

    yAxis.crossProduct(zAxis, xAxis);
    xf.set3X3Matrix(
      xAxis.x, xAxis.y, xAxis.z,
      yAxis.x, yAxis.y, yAxis.z,
      zAxis.x, zAxis.y, zAxis.z
    );

    s.scaledCopy(-1.0, eye);
    const t = Matrix4x4.createTranslationMatrix(s);
    return Matrix4x4.createTransComposeMatrix(xf, t);
  }

  public static createOrthogonalViewMatrix(left: number, right: number, bottom: number, top: number, near: number, far: number): Matrix4x4 {
    const xf = new Matrix4x4();

    xf.m[0][0] = 2.0 / (right - left);
    xf.m[0][3] = -(right + left) / (right - left);

    xf.m[1][1] = 2.0 / (top - bottom);
    xf.m[1][3] = -(top + bottom) / (top - bottom);

    xf.m[2][2] = -2.0 / (far - near);
    xf.m[2][3] = -(far + near) / (far - near);

    return xf;
  }

  public toString(): string {
    const rows = this.m.map((row) => `  [${row.join(", ")}]`);
    return `Matrix4x4{\n${rows.join("\n")}\n}`;
  }
}
