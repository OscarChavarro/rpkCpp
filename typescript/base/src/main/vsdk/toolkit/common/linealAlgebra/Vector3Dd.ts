export class Vector3Dd {
  public x: number;
  public y: number;
  public z: number;

  public constructor(inX = 0.0, inY = 0.0, inZ = 0.0) {
    this.x = inX;
    this.y = inY;
    this.z = inZ;
  }

  public distanceSquared(v2: Vector3Dd): number {
    const d = new Vector3Dd();
    d.x = v2.x - this.x;
    d.y = v2.y - this.y;
    d.z = v2.z - this.z;
    return d.x * d.x + d.y * d.y + d.z * d.z;
  }

  public dotProduct(b: Vector3Dd): number {
    return this.x * b.x + this.y * b.y + this.z * b.z;
  }

  public isNull(epsilon: number): boolean {
    return this.dotProduct(this) <= epsilon * epsilon;
  }

  public normalizeAndGivePreviousNorm(epsilon: number): number {
    let len = this.dotProduct(this);
    if (len <= 0.0) {
      return 0.0;
    }

    if (len <= 1.0 + epsilon && len >= 1.0 - epsilon) {
      len = 0.5 + 0.5 * len;
    }
    else {
      len = globalThis.Math.sqrt(len);
    }

    this.x /= len;
    this.y /= len;
    this.z /= len;

    return len;
  }

  public crossProduct(a: Vector3Dd, b: Vector3Dd): void {
    this.x = a.y * b.z - a.z * b.y;
    this.y = a.z * b.x - a.x * b.z;
    this.z = a.x * b.y - a.y * b.x;
  }

  public copy(source: Vector3Dd): void {
    this.x = source.x;
    this.y = source.y;
    this.z = source.z;
  }
}
