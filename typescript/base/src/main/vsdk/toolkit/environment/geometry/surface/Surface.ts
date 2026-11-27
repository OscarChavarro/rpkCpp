import { Geometry, type GeometryRay, type GeometryRayHit } from "../Geometry.js";
export abstract class Surface<
    TRay extends GeometryRay = GeometryRay,
    THit extends GeometryRayHit = GeometryRayHit,
> extends Geometry<TRay, THit> {}
