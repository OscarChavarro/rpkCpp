import type { Geometry } from "../../../skin/Geometry";
import type { Patch } from "./Patch";

/**
Patches and geometries that a visibility query must ignore, typically the
source and receiver of a form factor (and their twins), to avoid immediate
self-intersections. The value is built by the caller and passed down every
intersect call, so two queries never see each other's exclusions.

An excluded geometry (a source or receiver cluster) excludes every face
below it, wherever the spatial index stores that face: as part of the
geometry, of a sub-geometry, or as a loose patch. Since faces are numbered
depth-first over the cluster hierarchy (Scene.buildFaceTable()), the faces
below a geometry are a contiguous interval of face indices, so the face test
is two comparisons per excluded geometry and needs no references.

Shared, read-only: it has no setters, and every query is const.
*/
export class RayExclusionSet {
  public static readonly MAX_EXCLUDED_PATCHES = 4;

  private readonly excludedPatch0: Patch | null;
  private readonly excludedPatch1: Patch | null;
  private readonly excludedPatch2: Patch | null;
  private readonly excludedPatch3: Patch | null;
  private readonly excludedGeometry1: Geometry | null;
  private readonly excludedGeometry2: Geometry | null;
  // Face index intervals of the excluded geometries, empty when first > last
  private readonly excludedFirstFace1: number;
  private readonly excludedLastFace1: number;
  private readonly excludedFirstFace2: number;
  private readonly excludedLastFace2: number;

  /**
  Excludes up to four patches and up to two geometries. The face index
  intervals of the geometries are taken at construction time, so the face
  table must have been built (Scene.buildFaceTable()) before
  */
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
    this.excludedFirstFace1 = geometry1 !== null ? geometry1.getFirstFaceIndex() : 0;
    this.excludedLastFace1 = geometry1 !== null ? geometry1.getLastFaceIndex() : -1;
    this.excludedFirstFace2 = geometry2 !== null ? geometry2.getFirstFaceIndex() : 0;
    this.excludedLastFace2 = geometry2 !== null ? geometry2.getLastFaceIndex() : -1;
  }

  public getExcludedPatch(index: number): Patch | null {
    switch (index) {
      case 0:
        return this.excludedPatch0;
      case 1:
        return this.excludedPatch1;
      case 2:
        return this.excludedPatch2;
      case 3:
        return this.excludedPatch3;
      default:
        return null;
    }
  }

  public getExcludedGeometry1(): Geometry | null {
    return this.excludedGeometry1;
  }

  public getExcludedGeometry2(): Geometry | null {
    return this.excludedGeometry2;
  }

  public excludesPatch(patch: Patch): boolean {
    return this.excludedPatch0 === patch || this.excludedPatch1 === patch
      || this.excludedPatch2 === patch || this.excludedPatch3 === patch;
  }

  public excludesGeometry(geometry: Geometry): boolean {
    return geometry === this.excludedGeometry1 || geometry === this.excludedGeometry2;
  }

  /**
  True when the face lies below one of the excluded geometries
  */
  public excludesFace(faceIndex: number): boolean {
    return (faceIndex >= this.excludedFirstFace1 && faceIndex <= this.excludedLastFace1)
      || (faceIndex >= this.excludedFirstFace2 && faceIndex <= this.excludedLastFace2);
  }
}
