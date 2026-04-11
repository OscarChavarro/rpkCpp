export class Vector2D {
  public x: number;
  public y: number;

  public constructor(x = 0.0, y = 0.0) {
    this.x = x;
    this.y = y;
  }

  public static difference(a: Vector2D, b: Vector2D, o: Vector2D): void {
    o.x = a.x - b.x;
    o.y = a.y - b.y;
  }

  public static norm2(d: Vector2D): number {
    return d.x * d.x + d.y * d.y;
  }

  public toString(): string {
    return `Vector2D{x=${this.x}, y=${this.y}}`;
  }
}
