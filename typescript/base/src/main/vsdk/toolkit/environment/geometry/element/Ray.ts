import { FundamentalEntity } from "../../../common/FundamentalEntity.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { VSDK } from "../../../common/VSDK.js";
import { RaytraceStatistics } from "../../../common/statistics/RaytraceStatistics.js";
import type { GeometryRay } from "./GeometryRay.js";
import { Double } from "../../../../../java/lang/Double.js";
import { Objects } from "../../../../../java/util/Objects.js";

/**
Java's private four-argument constructor `Ray(origin, direction, t,
directionAlreadyNormalized)`: passing this token as the fourth argument
selects it, so it stays out of the public overloads.
*/
const DIRECTION_ALREADY_NORMALIZED: unique symbol = Symbol("Ray.directionAlreadyNormalized");

/**
 This class models a mathematical RAY.
 */
export class Ray extends FundamentalEntity implements GeometryRay {
    private static readonly UNIT_DIRECTION_TOLERANCE = 1e-12;

    private readonly origin: Vector3Dd;
    private readonly direction: Vector3Dd;
    private readonly t: number;

    public constructor();
    public constructor(origin: Vector3Dd, direction: Vector3Dd);
    public constructor(origin: Vector3Dd, direction: Vector3Dd, t: number);
    public constructor(b: Ray);
    public constructor(
        a?: Vector3Dd | Ray | null,
        direction?: Vector3Dd | null,
        t?: number,
        directionAlreadyNormalized?: typeof DIRECTION_ALREADY_NORMALIZED,
    ) {
        super();
        let origin: Vector3Dd | null | undefined;
        let normalized: boolean;
        if (arguments.length === 0) {
            // this(new Vector3Dd(0, 0, 0), new Vector3Dd(1, 0, 0), 0.0)
            origin = new Vector3Dd(0, 0, 0);
            direction = new Vector3Dd(1, 0, 0);
            t = 0.0;
            normalized = false;
        } else if (arguments.length === 1) {
            // Ray(Ray b)
            const b: Ray = Objects.requireNonNull(a as Ray | null, "Ray to copy cannot be null");
            origin = b.origin;
            direction = b.direction;
            t = b.t;
            normalized = true;
        } else {
            origin = a as Vector3Dd | null;
            if (t === undefined) {
                t = 0.0;
            }
            normalized = directionAlreadyNormalized === DIRECTION_ALREADY_NORMALIZED;
        }
        this.origin = Objects.requireNonNull(origin, "Ray origin cannot be null");
        this.direction = normalized
            ? Objects.requireNonNull(direction, "Ray direction cannot be null")
            : Ray.normalizeDirection(direction ?? null);
        this.t = t;
    }

    /**
    Explicit affine construction: preserve direction magnitude and t.
    Legacy constructors retain their historical normalization behavior.
    */
    public static fromAffine(origin: Vector3Dd, direction: Vector3Dd, t: number = 0.0): Ray {
        return Ray.withComponents(origin, direction, t);
    }

    public static copyOf(other: Ray): Ray {
        return Objects.requireNonNull(other, "Ray to copy cannot be null");
    }

    public withOrigin(newOrigin: Vector3Dd): Ray {
        return Ray.withComponents(newOrigin, this.direction, this.t);
    }

    // Copies replace only the named component; they never renormalize.
    public withDirection(newDirection: Vector3Dd): Ray {
        return Ray.withComponents(this.origin, newDirection, this.t);
    }

    public withT(newT: number): Ray {
        RaytraceStatistics.recordRayWithT();
        return Ray.withComponents(this.origin, this.direction, newT);
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

    /** `new Ray(origin, direction, t, true)`. */
    private static withComponents(origin: Vector3Dd, direction: Vector3Dd, t: number): Ray {
        return new (
            Ray as unknown as new (
                origin: Vector3Dd,
                direction: Vector3Dd,
                t: number,
                directionAlreadyNormalized: typeof DIRECTION_ALREADY_NORMALIZED,
            ) => Ray
        )(origin, direction, t, DIRECTION_ALREADY_NORMALIZED);
    }

    private static normalizeDirection(direction: Vector3Dd | null): Vector3Dd {
        const candidate: Vector3Dd = Objects.requireNonNull(direction, "Ray direction cannot be null");
        const lengthSquared: number = candidate.dotProduct(candidate);
        if (lengthSquared <= VSDK.EPSILON) {
            return candidate;
        }
        if (Math.abs(lengthSquared - 1.0) <= Ray.UNIT_DIRECTION_TOLERANCE) {
            return candidate;
        }
        return candidate.multiply(1.0 / Math.sqrt(lengthSquared));
    }

    public equals(obj: unknown): boolean {
        if (this === obj) {
            return true;
        }
        if (!(obj instanceof Ray)) {
            return false;
        }
        const other: Ray = obj;
        return (
            Double.compare(this.t, other.t) === 0 &&
            this.origin.equals(other.origin) &&
            this.direction.equals(other.direction)
        );
    }

    /** `Objects.hash(origin, direction, t)`. */
    public hashCode(): number {
        let result = 1;
        result = (Math.imul(31, result) + this.origin.hashCode()) | 0;
        result = (Math.imul(31, result) + this.direction.hashCode()) | 0;
        return (Math.imul(31, result) + Double.hashCode(this.t)) | 0;
    }

    /**
    Provides an object to text report conversion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human-readable representation of current Ray
    */
    public override toString(): string {
        return (
            "Ray Origin: " +
            this.origin.toString() +
            "; Direction: " +
            this.direction.toString() +
            " T: " +
            VSDK.formatDouble(this.t)
        );
    }
}
