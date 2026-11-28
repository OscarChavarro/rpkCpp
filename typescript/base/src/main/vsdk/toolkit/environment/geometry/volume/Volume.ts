import { Geometry, type GeometryRay, type GeometryRayHit } from "../Geometry.js";
import type { PolyhedralBoundedSolid } from "./polyhedralBoundedSolid/PolyhedralBoundedSolid.js";

/** Base for volumetric geometries. B-rep export becomes available for compatible concrete volumes. */
export abstract class Volume<
    TRay extends GeometryRay = GeometryRay,
    THit extends GeometryRayHit = GeometryRayHit,
> extends Geometry<TRay, THit> {
    public exportToPolyhedralBoundedSolid(): PolyhedralBoundedSolid | null {
        return null;
    }
}
