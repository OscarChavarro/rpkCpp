import { SurfaceDetail } from "./SurfaceDetail.js";
import { FundamentalEntity } from "../../../common/FundamentalEntity.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import type { Image } from "../../../media/Image.js";
import type { NormalMap } from "../../../media/NormalMap.js";
import type { SimpleMaterial } from "../../material/SimpleMaterial.js";
import type { Ray } from "./Ray.js";
import { RaytraceStatistics } from "../../../common/statistics/RaytraceStatistics.js";
import type { GeometryRayHit } from "../Geometry.js";
/**
RayHit describes the result of a ray/geometry intersection.
*/
export class RayHit extends FundamentalEntity implements GeometryRayHit {
    public static readonly DETAIL_NONE = SurfaceDetail.NONE;
    public static readonly DETAIL_POINT = SurfaceDetail.POINT;
    public static readonly DETAIL_NORMAL = SurfaceDetail.NORMAL;
    public static readonly DETAIL_UV = SurfaceDetail.UV;
    public static readonly DETAIL_TANGENT = SurfaceDetail.TANGENT;
    public static readonly DETAIL_ALL = SurfaceDetail.ALL;
    private static readonly ZERO_VECTOR: Vector3Dd = new Vector3Dd();

    public point!: Vector3Dd; // Intersection point coordinates
    public normal!: Vector3Dd; // Surface normal at intersection point
    public tangent!: Vector3Dd; // Surface tangent at intersection point
    // Note that surface binormal at intersection point must be calculated
    // by the application as the cross product (n x t).
    public u!: number; // Texture coordinate of intersection point
    public v!: number;
    // This can be null.
    public material!: SimpleMaterial | null; // Internal geometry selected material
    // This can be null.
    public texture!: Image | null; // Internal geometry selected texture map
    // This can be null.
    public normalMap!: NormalMap | null; // Internal geometry selected normal map
    // This can be null for miss results.
    private ray!: Ray | null; // Intersected ray with valid t at hit point
    private requiredDetailMask: number;
    private availableDetailMask!: number;
    private detailAvailabilityKnown!: boolean;
    private storeRay: boolean;
    private hitDistance!: number;
    private hitDistanceKnown!: boolean;

    public constructor();
    public constructor(requiredDetailMask: number);
    public constructor(requiredDetailMask: number, storeRay: boolean);
    public constructor(other: RayHit);
    public constructor(a: number | RayHit = RayHit.DETAIL_ALL, storeRay: boolean = true) {
        super();
        const other: RayHit | null = a instanceof RayHit ? a : null;
        // RayHit(RayHit other): this(other.requiredDetailMask), then clone(other)
        this.requiredDetailMask = other !== null ? other.requiredDetailMask : (a as number);
        this.storeRay = other !== null ? true : storeRay;
        this.clear();
        RaytraceStatistics.recordRayHitInstance();
        if (other !== null) {
            this.clone(other);
        }
    }

    public clear(): void {
        this.point = RayHit.ZERO_VECTOR;
        this.normal = RayHit.ZERO_VECTOR;
        this.tangent = RayHit.ZERO_VECTOR;
        this.u = 0;
        this.v = 0;
        this.material = null;
        this.texture = null;
        this.normalMap = null;
        this.ray = null;
        this.availableDetailMask = RayHit.DETAIL_NONE;
        this.detailAvailabilityKnown = false;
        this.hitDistance = 0;
        this.hitDistanceKnown = false;
    }

    public reset(newRequiredDetailMask: number): void {
        this.requiredDetailMask = newRequiredDetailMask;
        this.clear();
    }

    public resetForDistanceOnly(): void {
        this.requiredDetailMask = RayHit.DETAIL_NONE;
        this.ray = null;
        this.availableDetailMask = RayHit.DETAIL_NONE;
        this.detailAvailabilityKnown = false;
        this.hitDistance = 0;
        this.hitDistanceKnown = false;
    }

    /**
    `clone(other)` is Java's `clone(RayHit)`. `clone()` is the one RayHit
    inherits from `Entity`.
    */
    public override clone(): unknown;
    public override clone(other: RayHit): void;
    public override clone(other?: RayHit): unknown {
        if (other === undefined) {
            return super.clone();
        }
        RaytraceStatistics.recordHitInfoClone();
        this.requiredDetailMask = other.requiredDetailMask;
        this.storeRay = other.storeRay;
        this.availableDetailMask = other.availableDetailMask;
        this.detailAvailabilityKnown = other.detailAvailabilityKnown;
        this.hitDistance = other.hitDistance;
        this.hitDistanceKnown = other.hitDistanceKnown;
        this.point = other.point;
        this.normal = other.normal;
        this.tangent = other.tangent;
        this.u = other.u;
        this.v = other.v;
        this.material = other.material;
        this.texture = other.texture;
        this.normalMap = other.normalMap;
        this.ray = other.ray;
        return undefined;
    }

    // Legacy producers may leave availability unknown; requested is not available.
    public isDetailAvailabilityKnown(): boolean {
        return this.detailAvailabilityKnown;
    }
    public getAvailableDetailMask(): number {
        return this.availableDetailMask;
    }
    public beginDetailEvaluation(): void {
        this.availableDetailMask = RayHit.DETAIL_NONE;
        this.detailAvailabilityKnown = true;
    }
    public markDetailAvailable(mask: number): void {
        this.availableDetailMask |= mask & RayHit.DETAIL_ALL;
    }
    public getRequiredDetailMask() {
        return this.requiredDetailMask;
    }
    public setRequiredDetailMask(value: number) {
        this.requiredDetailMask = value;
    }
    public shouldStoreRay() {
        return this.storeRay;
    }
    public setStoreRay(value: boolean) {
        this.storeRay = value;
    }
    public needsPoint() {
        return (this.requiredDetailMask & RayHit.DETAIL_POINT) !== 0;
    }
    public needsNormal() {
        return (this.requiredDetailMask & RayHit.DETAIL_NORMAL) !== 0;
    }
    public needsTextureCoordinates() {
        return (this.requiredDetailMask & RayHit.DETAIL_UV) !== 0;
    }
    public needsTangent() {
        return (this.requiredDetailMask & RayHit.DETAIL_TANGENT) !== 0;
    }
    public needsAnySurfaceData() {
        return this.requiredDetailMask !== RayHit.DETAIL_NONE;
    }
    public getRay() {
        return this.ray;
    }
    public setRay(value: Ray | null) {
        this.ray = value;
        if (value !== null) {
            this.hitDistance = value.getT();
            this.hitDistanceKnown = true;
        }
    }
    public hasHitDistance() {
        return this.hitDistanceKnown;
    }
    public getHitDistance() {
        if (this.hitDistanceKnown) {
            return this.hitDistance;
        }
        if (this.ray !== null) {
            return this.ray.getT();
        }
        return 0;
    }
    public setHitDistance(value: number) {
        this.hitDistance = value;
        this.hitDistanceKnown = true;
    }
}
