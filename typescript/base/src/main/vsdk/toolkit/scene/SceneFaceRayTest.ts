import type { Ray as RpkRay } from "../common/linealAlgebra/Ray";
import type { RayExclusionSet } from "../environment/geometry/elements/RayExclusionSet";
import type { RayHit } from "../environment/geometry/elements/RayHit";
import { PrimitiveRayTest } from "../environment/geometry/spatialIndex/PrimitiveRayTest";
import type { Scene } from "./Scene";

/**
RenderPark's exact face test as the PrimitiveRayTest of a VITRAL
SpatialIndex: primitive i is face i of the scene's face table, tested with
Patch.intersect on the original ray, flags and exclusions, so an index
answer is decided by the same arithmetic as the voxel grid's, the exclusions
included (a face below an excluded geometry is rejected by its face-index
range, plan decision D5).

One object per query: it writes the accepted hit into the caller's hit
record, so after a closest-hit query the record holds the closest face.
*/
export class SceneFaceRayTest extends PrimitiveRayTest {
  private readonly scene: Scene;
  private readonly ray: RpkRay;
  private readonly hitFlags: number;
  private readonly exclusions: RayExclusionSet;
  private readonly hitStore: RayHit | null;
  private readonly distance: number[];

  public constructor(
    inScene: Scene,
    inRay: RpkRay,
    inHitFlags: number,
    inExclusions: RayExclusionSet,
    inHitStore: RayHit | null
  ) {
    super();
    this.scene = inScene;
    this.ray = inRay;
    this.hitFlags = inHitFlags;
    this.exclusions = inExclusions;
    this.hitStore = inHitStore;
    this.distance = [0.0];
  }

  public override test(primitive: number, minimumT: number, maximumT: number, hitT: number[]): boolean {
    this.distance[0] = Math.fround(maximumT);
    const hit = this.scene.getFace(primitive).intersect(
      this.ray, Math.fround(minimumT), this.distance, this.hitFlags, this.hitStore, this.exclusions);
    if (hit === null) {
      return false;
    }
    hitT[0] = this.distance[0]!;
    return true;
  }
}
