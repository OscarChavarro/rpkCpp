import { Vector3Dd } from "./Vector3Dd";

type Matrix4d = [
  [number, number, number, number],
  [number, number, number, number],
  [number, number, number, number],
  [number, number, number, number]
];

export class Matrix4x4d {
  public m: Matrix4d;

  public constructor() {
    this.m = [
      [1.0, 0.0, 0.0, 0.0],
      [0.0, 1.0, 0.0, 0.0],
      [0.0, 0.0, 1.0, 0.0],
      [0.0, 0.0, 0.0, 1.0]
    ];
  }

  public multiply(v3a: Vector3Dd, v3b: Vector3Dd): void {
    const tmp = new Matrix4x4d();

    tmp.m[0][0] = v3b.x * this.m[0][0] + v3b.y * this.m[1][0] + v3b.z * this.m[2][0];
    tmp.m[0][1] = v3b.x * this.m[0][1] + v3b.y * this.m[1][1] + v3b.z * this.m[2][1];
    tmp.m[0][2] = v3b.x * this.m[0][2] + v3b.y * this.m[1][2] + v3b.z * this.m[2][2];

    v3a.x = tmp.m[0][0]!;
    v3a.y = tmp.m[0][1]!;
    v3a.z = tmp.m[0][2]!;
  }

  public multiplyWithTranslation(p3a: Vector3Dd, p3b: Vector3Dd): void {
    this.multiply(p3a, p3b);
    p3a.x += this.m[3][0]!;
    p3a.y += this.m[3][1]!;
    p3a.z += this.m[3][2]!;
  }

  public copy(source: Matrix4x4d): void {
    for (let i = 0; i < 4; i++) {
      for (let j = 0; j < 4; j++) {
        this.m[i]![j] = source.m[i]![j]!;
      }
    }
  }

  public identity(): void {
    const tmp = new Matrix4x4d();
    this.copy(tmp);
  }

  public static multiplyMatrix4(m4a: Matrix4x4d, m4b: Matrix4x4d, m4c: Matrix4x4d): void {
    const tmp = new Matrix4x4d();
    for (let i = 3; i >= 0; i--) {
      for (let j = 3; j >= 0; j--) {
        tmp.m[i]![j] =
          m4b.m[i]![0]! * m4c.m[0]![j]! +
          m4b.m[i]![1]! * m4c.m[1]![j]! +
          m4b.m[i]![2]! * m4c.m[2]![j]! +
          m4b.m[i]![3]! * m4c.m[3]![j]!;
      }
    }

    m4a.copy(tmp);
  }
}
