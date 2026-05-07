import { Numeric } from "../common/linealAlgebra/Numeric";
import { Ray } from "../common/linealAlgebra/Ray";
import { RayHitFlag } from "../skin/RayHitFlag";
import { Patch } from "../skin/Patch";
import { RayHit } from "../skin/RayHit";

export class ShadowCache {
  private static readonly MAX_CACHE = 5;
  private readonly patchCache: Array<Patch | null>;
  private numberOfCachedPatches: number;

  public constructor() {
    this.numberOfCachedPatches = 0;
    this.patchCache = new Array<Patch | null>(ShadowCache.MAX_CACHE).fill(null);
  }

  public cacheHit(ray: Ray, distance: number[], hitStore: RayHit): RayHit | null {
    for (let i = 0; i < this.numberOfCachedPatches; i++) {
      if (this.patchCache[i] === null) {
        continue;
      }
      const hit = (this.patchCache[i] as Patch).intersect(
        ray,
        Numeric.EPSILON_FLOAT * distance[0],
        distance,
        RayHitFlag.FRONT | RayHitFlag.ANY,
        hitStore,
      );
      if (hit !== null) {
        return hit;
      }
    }
    return null;
  }

  public addToShadowCache(patch: Patch): void {
    this.patchCache[this.numberOfCachedPatches % ShadowCache.MAX_CACHE] = patch;
    if (this.numberOfCachedPatches < ShadowCache.MAX_CACHE) {
      this.numberOfCachedPatches++;
    }
  }
}
