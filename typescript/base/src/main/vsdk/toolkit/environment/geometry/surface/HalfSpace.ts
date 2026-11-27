import { Surface } from "./Surface.js";
import type { GeometryRay, GeometryRayHit } from "../Geometry.js";
export abstract class HalfSpace<
    TRay extends GeometryRay = GeometryRay,
    THit extends GeometryRayHit = GeometryRayHit,
> extends Surface<TRay, THit> {}
