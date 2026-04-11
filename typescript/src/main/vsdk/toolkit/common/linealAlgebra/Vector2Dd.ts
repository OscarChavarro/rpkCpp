export class Vector2Dd {
  public u: number;
  public v: number;

  public constructor() {
    this.u = 0.0;
    this.v = 0.0;
  }

  public static set(vector: Vector2Dd, a: number, b: number): void {
    vector.u = a;
    vector.v = b;
  }

  public static subtract(p: Vector2Dd, q: Vector2Dd, r: Vector2Dd): void {
    r.u = p.u - q.u;
    r.v = p.v - q.v;
  }

  public static add(p: Vector2Dd, q: Vector2Dd, r: Vector2Dd): void {
    r.u = p.u + q.u;
    r.v = p.v + q.v;
  }

  public static negate(p: Vector2Dd): void {
    p.u = -p.u;
    p.v = -p.v;
  }

  public static determinant(a: Vector2Dd, b: Vector2Dd): number {
    return a.u * b.v - a.v * b.u;
  }
}
