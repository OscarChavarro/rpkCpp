import type { GeometryRay } from "./element/GeometryRay.js";
import type { Ray } from "./element/Ray.js";
import type { RayCrossingQuery } from "./element/RayCrossingQuery.js";
import type { RayCrossingSink } from "./element/RayCrossingSink.js";
import { RayCrossingSupport } from "./element/RayCrossingSupport.js";
import { RaySolidQueryStatus } from "./element/RaySolidQueryStatus.js";
import { RayFirstHitStatus } from "./element/RayFirstHitStatus.js";
import { GeometryContainment } from "./element/GeometryContainment.js";
import type { GeometryBounds } from "./element/GeometryBounds.js";
import { RayBoundary as RayBoundaryValue, type RayBoundary } from "./element/RayBoundary.js";
import type { RaySolidClassification } from "./element/RaySolidClassification.js";
import type { RayHit } from "./element/RayHit.js";
import { Entity } from "../../common/Entity.js";
import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd.js";
import { Containment } from "../../processing/Containment.js";

/** Structural intersection contracts implemented by Phase 19 ray elements. */
export type { GeometryRay };
export interface GeometryRayHit {}

/** Root contract shared by all origin-centered Vitral geometries. */
export abstract class Geometry<
    TRay extends GeometryRay = GeometryRay,
    THit extends GeometryRayHit = GeometryRayHit,
> extends Entity {
    public static readonly INSIDE = Containment.INSIDE;
    public static readonly LIMIT = Containment.LIMIT;
    public static readonly OUTSIDE = Containment.OUTSIDE;

    // An independent mathematical copy, or null when copying is unsupported.
    public copyGeometry(): Geometry | null {
        return null;
    }

    // Unsupported is distinct from an empty sequence of crossings.
    public emitRayCrossings(_ray: Ray, _query: RayCrossingQuery, _sink: RayCrossingSink): RayCrossingSupport {
        return RayCrossingSupport.Unsupported;
    }

    // Full solid membership over the query interval; never infer interior from
    // surface hits alone. Unsupported geometries leave result empty.
    public classifyRay(_ray: Ray, _query: RayCrossingQuery, result: RaySolidClassification): RaySolidQueryStatus {
        result.clear();
        return RaySolidQueryStatus.Unsupported;
    }

    // Inclusive explicit interval; unsupported implementations clear output.
    public queryFirstHit(
        _ray: Ray,
        _query: RayCrossingQuery,
        result: RayHit | null,
        boundary: RayBoundary | null = null,
    ): RayFirstHitStatus {
        if (result) result.clear();
        if (boundary) boundary.assign(new RayBoundaryValue());
        return RayFirstHitStatus.Unsupported;
    }

    // Resolve detail for a boundary in this geometry's local coordinates.
    // Hit means evaluated; unsupported and numerical failures remain explicit.
    public querySurfaceDetail(_ray: Ray, _boundary: RayBoundary, result: RayHit | null): RayFirstHitStatus {
        if (result) result.clear();
        return RayFirstHitStatus.Unsupported;
    }

    /** Updates the supplied hit record when this geometry has a first ray hit. */
    public abstract doIntersectionFirstHit(inRay: TRay, outHit: THit): boolean;

    /**
     * Default geometries have no additional hit attributes. Concrete surfaces
     * override this to populate point, normal, UV, or tangent information.
     */
    public doExtraInformation(_inRay: TRay, _inT: number, _outHit: THit): void {}

    /** Default transparent-geometry visibility classification. */
    public computeQuantitativeInvisibility(_origin: Vector3Dd, _p: Vector3Dd): number {
        return 0;
    }

    // Unsupported is explicit; legacy getMinMax remains available.
    public getBounds(result: GeometryBounds): void {
        result.setUnsupported();
    }

    /** Axis-aligned local bounds: min xyz followed by max xyz. */
    public abstract getMinMax(): Float64Array | number[];

    // Typed containment; unsupported is explicit here.
    public classifyPoint(_p: Vector3Dd, _distanceTolerance: number): GeometryContainment {
        return GeometryContainment.Unsupported;
    }
    // Certified local-space classification of the closed ball of radius
    // distanceTolerance around the point. Inside/Outside: the whole ball lies
    // strictly in the interior/exterior. Boundary: a boundary point lies within
    // the ball. Anything not proven, including at zero tolerance, is Indeterminate.
    public classifyPointCertified(_p: Vector3Dd, _distanceTolerance: number): GeometryContainment {
        return GeometryContainment.Unsupported;
    }
    // Certified classification of the closed axis-aligned local box
    // center +/- halfWidths: Inside/Outside only when the whole box lies strictly
    // in the interior/exterior; never Boundary. Other results are Indeterminate.
    public classifyBoxCertified(_center: Vector3Dd, _halfWidths: Vector3Dd): GeometryContainment {
        return GeometryContainment.Unsupported;
    }
    // Legacy integer predicate kept for parity with the Java port. It is not
    // the typed contract: its tolerance band is family specific, the default
    // reports OUTSIDE for unsupported geometries, and surfaces report LIMIT only
    // on the surface. Typed scene and CSG queries never consult it. Adapters for
    // solids introduced with the typed contract raise when a typed result has
    // no integer representation.
    /** Default classification for non-solid geometries. */
    public doContainmentTest(_p: Vector3Dd, _distanceTolerance: number): number {
        return Geometry.OUTSIDE;
    }
}
