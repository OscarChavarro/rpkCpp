import { VSDK } from "../../../common/VSDK";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd";

/**
This class models a mathematical RAY.
*/
export class Ray {
  private static readonly UNIT_DIRECTION_TOLERANCE = 1e-12;
  private static readonly hashBuffer = new DataView(new ArrayBuffer(8));

  private origin: Vector3Dd;
  private direction: Vector3Dd;
  private t: number;

  private static normalizeDirection(direction: Vector3Dd): Vector3Dd {
    const lengthSquared = direction.dotProduct(direction);
    if (lengthSquared <= VSDK.EPSILON) {
      return direction;
    }
    if (globalThis.Math.abs(lengthSquared - 1.0) <= Ray.UNIT_DIRECTION_TOLERANCE) {
      return direction;
    }
    return direction.multiply(1.0 / globalThis.Math.sqrt(lengthSquared));
  }

  public constructor();
  public constructor(origin: Vector3Dd, direction: Vector3Dd);
  public constructor(origin: Vector3Dd, direction: Vector3Dd, t: number);
  public constructor(origin?: Vector3Dd, direction?: Vector3Dd, t = 0.0) {
    this.origin = origin ?? new Vector3Dd(0, 0, 0);
    this.direction = Ray.normalizeDirection(direction ?? new Vector3Dd(1, 0, 0));
    this.t = t;
  }

  /**
  Explicit affine construction: preserve direction magnitude and t.
  Legacy constructors retain their historical normalization behavior.
  */
  public static fromAffine(origin: Vector3Dd, direction: Vector3Dd, t = 0.0): Ray {
    const result = new Ray();
    result.setOriginAndDirection(origin, direction);
    result.setT(t);
    return result;
  }

  public static copyOf(other: Ray): Ray {
    const result = new Ray();
    result.origin = other.origin;
    result.direction = other.direction;
    result.t = other.t;
    return result;
  }

  /** Copies replace only the named component; they never renormalize */
  public withOrigin(newOrigin: Vector3Dd): Ray {
    const result = Ray.copyOf(this);
    result.origin = newOrigin;
    return result;
  }

  public withDirection(newDirection: Vector3Dd): Ray {
    const result = Ray.copyOf(this);
    result.direction = newDirection;
    return result;
  }

  public withT(newT: number): Ray {
    const result = Ray.copyOf(this);
    result.t = newT;
    return result;
  }

  public getOrigin(): Vector3Dd {
    return this.origin;
  }

  public getDirection(): Vector3Dd {
    return this.direction;
  }

  public getT(): number {
    return this.t;
  }

  public setOrigin(newOrigin: Vector3Dd): void {
    this.origin = newOrigin;
  }

  public setDirection(newDirection: Vector3Dd): void {
    this.direction = newDirection;
  }

  public setT(newT: number): void {
    this.t = newT;
  }

  public setOriginAndDirection(newOrigin: Vector3Dd, newDirection: Vector3Dd): void {
    this.origin = newOrigin;
    this.direction = newDirection;
  }

  public equals(other: Ray): boolean {
    return this.t === other.t && this.origin.equals(other.origin) && this.direction.equals(other.direction);
  }

  private static hashDouble(value: number): number {
    Ray.hashBuffer.setFloat64(0, value, true);
    let h = 0;
    for (let i = 0; i < 8; i++) {
      h = (globalThis.Math.imul(h, 31) + Ray.hashBuffer.getUint8(i)) >>> 0;
    }
    return h;
  }

  public hashCode(): number {
    let result = Ray.hashDouble(this.t);
    result = (globalThis.Math.imul(31, result) + (this.origin.hashCode() >>> 0)) >>> 0;
    result = (globalThis.Math.imul(31, result) + (this.direction.hashCode() >>> 0)) >>> 0;
    return result | 0;
  }

  /**
  Provides an object to text report conversion, optimized for human
  readability and debugging. Do not use for serialization or persistence
  purposes.
  @return human-readable representation of current Ray
  */
  public toString(): string {
    return "Ray Origin: <" + VSDK.formatDouble(this.origin.x()) + ", "
      + VSDK.formatDouble(this.origin.y()) + ", "
      + VSDK.formatDouble(this.origin.z()) + ">; Direction: <"
      + VSDK.formatDouble(this.direction.x()) + ", "
      + VSDK.formatDouble(this.direction.y()) + ", "
      + VSDK.formatDouble(this.direction.z()) + "> T: "
      + VSDK.formatDouble(this.t);
  }
}
