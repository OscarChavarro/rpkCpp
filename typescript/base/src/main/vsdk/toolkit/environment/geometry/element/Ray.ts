import { FundamentalEntity } from "../../../common/FundamentalEntity.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { VSDK } from "../../../common/VSDK.js";
import type { GeometryRay } from "./GeometryRay.js";
import { Double } from "../../../../../java/lang/Double.js";
export class Ray extends FundamentalEntity implements GeometryRay {
    private static readonly UNIT_DIRECTION_TOLERANCE = 1e-12;
    private readonly origin: Vector3Dd;
    private readonly direction: Vector3Dd;
    private readonly t: number;
    public constructor();
    public constructor(origin: Vector3Dd, direction: Vector3Dd, t?: number);
    public constructor(ray: Ray);
    public constructor(a: Vector3Dd | Ray = new Vector3Dd(), b = new Vector3Dd(1, 0, 0), t = 0) {
        super();
        if (a instanceof Ray) {
            this.origin = a.origin;
            this.direction = a.direction;
            this.t = a.t;
        } else {
            this.origin = a;
            this.direction = Ray.normalizeDirection(b);
            this.t = t;
        }
    }
    // Explicit affine construction: preserve direction magnitude and t.
    // Legacy constructors retain their historical normalization behavior.
    public static fromAffine(origin: Vector3Dd, direction: Vector3Dd, t = 0): Ray {
        return Ray.withComponents(origin, direction, t);
    }
    /** Builds a ray from components without normalizing, like the C++ member copies. */
    private static withComponents(origin: Vector3Dd, direction: Vector3Dd, t: number): Ray {
        const result = new Ray(origin, new Vector3Dd(1, 0, 0), t);
        (result as unknown as { direction: Vector3Dd }).direction = direction;
        return result;
    }
    public static copyOf(x: Ray) {
        if (x === null) throw new TypeError("Ray to copy cannot be null");
        return x;
    }
    // Copies replace only the named component; they never renormalize.
    public withOrigin(x: Vector3Dd) {
        return Ray.withComponents(x, this.direction, this.t);
    }
    public withDirection(x: Vector3Dd) {
        return Ray.withComponents(this.origin, x, this.t);
    }
    public withT(x: number) {
        return Ray.withComponents(this.origin, this.direction, x);
    }
    public getOrigin() {
        return this.origin;
    }
    public getDirection() {
        return this.direction;
    }
    public getT() {
        return this.t;
    }
    public equals(x: unknown) {
        return (
            x instanceof Ray &&
            Double.compare(this.t, x.t) === 0 &&
            this.origin.equals(x.origin) &&
            this.direction.equals(x.direction)
        );
    }
    public hashCode(): number {
        let result = 1;
        result = (Math.imul(31, result) + this.origin.hashCode()) | 0;
        result = (Math.imul(31, result) + this.direction.hashCode()) | 0;
        return (Math.imul(31, result) + Double.hashCode(this.t)) | 0;
    }
    private static normalizeDirection(direction: Vector3Dd): Vector3Dd {
        const lengthSquared = direction.dotProduct(direction);
        if (lengthSquared <= VSDK.EPSILON || Math.abs(lengthSquared - 1) <= this.UNIT_DIRECTION_TOLERANCE)
            return direction;
        return direction.multiply(1 / Math.sqrt(lengthSquared));
    }
    public override toString() {
        return `Ray Origin: ${this.origin}; Direction: ${this.direction} T: ${VSDK.formatDouble(this.t)}`;
    }
}
