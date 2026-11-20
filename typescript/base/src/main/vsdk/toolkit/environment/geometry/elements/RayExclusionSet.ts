import type { Geometry } from "../../../skin/Geometry";
import type { Patch } from "./Patch";

/**
Patches and geometries that a visibility query must ignore, typically the
source and receiver of a form factor (and their twins), to avoid immediate
self-intersections. The value is built by the caller and passed down every
intersect call, so two queries never see each other's exclusions.

Shared, read-only: it has no setters, and every query is const.
*/
export class RayExclusionSet {
  private readonly excludedPatch0: Patch | null;
  private readonly excludedPatch1: Patch | null;
  private readonly excludedPatch2: Patch | null;
  private readonly excludedPatch3: Patch | null;
  private readonly excludedGeometry1: Geometry | null;
  private readonly excludedGeometry2: Geometry | null;

  public constructor(
    p0: Patch | null = null,
    p1: Patch | null = null,
    p2: Patch | null = null,
    p3: Patch | null = null,
    geometry1: Geometry | null = null,
    geometry2: Geometry | null = null
  ) {
    this.excludedPatch0 = p0;
    this.excludedPatch1 = p1;
    this.excludedPatch2 = p2;
    this.excludedPatch3 = p3;
    this.excludedGeometry1 = geometry1;
    this.excludedGeometry2 = geometry2;
  }

  public excludesPatch(patch: Patch): boolean {
    return this.excludedPatch0 === patch || this.excludedPatch1 === patch
      || this.excludedPatch2 === patch || this.excludedPatch3 === patch;
  }

  public excludesGeometry(geometry: Geometry): boolean {
    return geometry === this.excludedGeometry1 || geometry === this.excludedGeometry2;
  }
}
