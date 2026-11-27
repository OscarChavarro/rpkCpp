import { ArrayList } from "vitral/dist/java/util/ArrayList";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Ray } from "vitral/dist/vsdk/toolkit/environment/geometry/element/Ray";
import type { RayExclusionSet } from "../environment/geometry/elements/RayExclusionSet";
import { RayHitFlag } from "../environment/geometry/elements/RayHitFlag";
import type { RayHit } from "../environment/geometry/elements/RayHit";
import { BinnedSahSpatialIndex } from "vitral/dist/vsdk/toolkit/environment/geometry/spatialIndex/BinnedSahSpatialIndex";
import { GridSpatialIndex } from "vitral/dist/vsdk/toolkit/environment/geometry/spatialIndex/GridSpatialIndex";
import { LinearSpatialIndex } from "vitral/dist/vsdk/toolkit/environment/geometry/spatialIndex/LinearSpatialIndex";
import { SpatialGroupTree } from "vitral/dist/vsdk/toolkit/environment/geometry/spatialIndex/SpatialGroupTree";
import type { SpatialIndex } from "vitral/dist/vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndex";
import { SpatialIndexWorkspace } from "vitral/dist/vsdk/toolkit/environment/geometry/spatialIndex/SpatialIndexWorkspace";
import { SpatialQueryMode } from "vitral/dist/vsdk/toolkit/environment/geometry/spatialIndex/SpatialQueryMode";
import type { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import type { Compound } from "../skin/Compound";
import { Geometry } from "../skin/Geometry";
import { SceneAccelerationMethod } from "./SceneAccelerationMethod";
import { SceneFaceRayTest } from "./SceneFaceRayTest";
import type { Scene } from "./Scene";

function boxValues(box: BoundingBox, values: number[]): void {
  values[0] = box.minX();
  values[1] = box.minY();
  values[2] = box.minZ();
  values[3] = box.maxX();
  values[4] = box.maxY();
  values[5] = box.maxZ();
}

/**
Adds the members of a geometry of the cluster hierarchy to its group, in the
order the legacy grid visits them: the children of a compound, the patches
(as their face indices) of any other geometry
*/
function addMembers(tree: SpatialGroupTree, group: number, geometry: Geometry): void {
  if (geometry.isCompound()) {
    const children = (geometry as unknown as Compound).children;
    for (let i = 0; children !== null && i < children.length; i++) {
      const child = children[i]!;
      const box = [0, 0, 0, 0, 0, 0];
      boxValues(child.boundingBox, box);
      addMembers(tree, tree.addGroup(group, child.bounded ? box : null), child);
    }
    return;
  }
  const patches = Geometry.patchListReference(geometry);
  for (let i = 0; patches !== null && i < patches.length; i++) {
    tree.addPrimitive(group, patches[i]!.getFaceIndex());
  }
}

/**
The scene's visibility query: the nearest (or, with RayHitFlag.ANY, some)
face that a ray hits within an interval, through the spatial index chosen by
a SceneAccelerationMethod. Faces are the entries of the scene's face table;
the exact face test, flags and exclusions are RenderPark's Patch.intersect
for every method, so the methods differ only in which faces they test.
An ANY query may return another blocker with each method; its visibility
answer is the same.

Shared, read-only after construction: intersect writes only to the hit
record and the workspace it is given. Built after the face table; a change
of the scene geometry requires a new query.
*/
export class SceneQuery {
  private readonly scene: Scene;
  private readonly method: SceneAccelerationMethod;
  private readonly index: SpatialIndex | null;

  public constructor(inScene: Scene, inMethod: SceneAccelerationMethod) {
    this.scene = inScene;
    this.method = inMethod;
    this.index = SceneQuery.buildIndex(this.scene, this.method);
  }

  public destroy(): void {
  }

  /**
  The VITRAL index of a method over the scene's face table, primitive i
  being face i, or null for LEGACY_VOXEL_ACCELERATION. WALD2007 gets the face
  boxes padded by the face tolerance plus a relative margin, so that each
  contains every point the face test may accept; the grid gets the unpadded
  boxes (it pads them as the legacy grid does) and the cluster hierarchy as
  its tree of groups.
  */
  public static buildIndex(scene: Scene, method: SceneAccelerationMethod): SpatialIndex | null {
    const faceBounds = scene.getFaceBounds()!;
    const numberOfFaces = faceBounds.getNumberOfFaces();

    if (method === SceneAccelerationMethod.BRUTE_FORCE_ACCELERATION) {
      return new LinearSpatialIndex(numberOfFaces);
    }

    if (method === SceneAccelerationMethod.WALD2007_ACCELERATION) {
      const boxes = new ArrayList<number>(6 * numberOfFaces);
      const low = [0, 0, 0];
      const high = [0, 0, 0];
      for (let i = 0; i < numberOfFaces; i++) {
        const box = faceBounds.get(i);
        const extent = box.maxExtent();
        low[0] = box.minX();
        low[1] = box.minY();
        low[2] = box.minZ();
        high[0] = box.maxX();
        high[1] = box.maxY();
        high[2] = box.maxZ();
        const tolerance = scene.getFace(i).getTolerance();
        for (let axis = 0; axis < 3; axis++) {
          const magnitude = globalThis.Math.max(globalThis.Math.abs(low[axis]!), globalThis.Math.abs(high[axis]!));
          const padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
          boxes.add(low[axis]! - padding);
        }
        for (let axis = 0; axis < 3; axis++) {
          const magnitude = globalThis.Math.max(globalThis.Math.abs(low[axis]!), globalThis.Math.abs(high[axis]!));
          const padding = tolerance + 1e-4 * extent + 1e-5 * magnitude;
          boxes.add(high[axis]! + padding);
        }
      }
      return new BinnedSahSpatialIndex(boxes, new ArrayList<number>());
    }

    if (method === SceneAccelerationMethod.VOXEL_ACCELERATION) {
      const rawBoxes = new ArrayList<number>(6 * numberOfFaces);
      const box = [0, 0, 0, 0, 0, 0];
      for (let i = 0; i < numberOfFaces; i++) {
        boxValues(faceBounds.get(i), box);
        for (let k = 0; k < 6; k++) {
          rawBoxes.add(box[k]!);
        }
      }
      const root = scene.clusteredRootGeometry as Geometry;
      const rootBox = [0, 0, 0, 0, 0, 0];
      boxValues(root.boundingBox, rootBox);
      const tree = new SpatialGroupTree(rawBoxes, rootBox);
      addMembers(tree, 0, root);
      return new GridSpatialIndex(tree);
    }

    return null;
  }

  /** The option name of a method: voxel, wald2007, bruteforce */
  public static methodName(method: SceneAccelerationMethod): string {
    switch (method) {
      case SceneAccelerationMethod.VOXEL_ACCELERATION:
        return "voxel";
      case SceneAccelerationMethod.WALD2007_ACCELERATION:
        return "wald2007";
      case SceneAccelerationMethod.BRUTE_FORCE_ACCELERATION:
        return "bruteforce";
    }
    return "unknown";
  }

  /**
  The method of an option name, written to method[0]; false when the name is
  none of them
  */
  public static methodFromName(name: string | null, method: SceneAccelerationMethod[]): boolean {
    const METHODS = [
      SceneAccelerationMethod.VOXEL_ACCELERATION,
      SceneAccelerationMethod.WALD2007_ACCELERATION,
      SceneAccelerationMethod.BRUTE_FORCE_ACCELERATION
    ];
    if (name === null) {
      return false;
    }
    for (let i = 0; i < 4; i++) {
      if (name === SceneQuery.methodName(METHODS[i]!)) {
        method[0] = METHODS[i]!;
        return true;
      }
    }
    return false;
  }

  public getMethod(): SceneAccelerationMethod {
    return this.method;
  }

  public getIndex(): SpatialIndex | null {
    return this.index;
  }

  /**
  As RenderPark's VoxelGrid.gridIntersect: returns hitStore filled with the
  accepted hit and sets maximumDistance[0] to its distance, or returns null
  and leaves both unchanged when no face is hit in
  [minimumDistance, maximumDistance[0]]
  */
  public intersect(
    ray: Ray,
    minimumDistance: number,
    maximumDistance: number[],
    hitFlags: number,
    hitStore: RayHit,
    exclusions: RayExclusionSet,
    workspace: SceneQueryWorkspace
  ): RayHit | null {
    const test = new SceneFaceRayTest(this.scene, ray, hitFlags, exclusions, hitStore);
    const indexRay = Ray.fromAffine(ray.getOrigin(), ray.getDirection());
    const mode = (hitFlags & RayHitFlag.ANY) !== 0
      ? SpatialQueryMode.ANY_HIT : SpatialQueryMode.CLOSEST_HIT;
    const result = this.index!.query(
      indexRay, minimumDistance, maximumDistance[0]!, mode, test, workspace.getIndexWorkspace()!);
    if (!result.hasHit()) {
      return null;
    }
    maximumDistance[0] = result.getParameter();
    return hitStore;
  }
}

/**
Per-query scratch of a SceneQuery: the workspace of its index.

Workspace: mutable, owned by one caller and passed explicitly on every
SceneQuery.intersect call.
*/
export class SceneQueryWorkspace {
  private readonly indexWorkspace: SpatialIndexWorkspace | null;

  public constructor(query: SceneQuery) {
    this.indexWorkspace = new SpatialIndexWorkspace(query.getIndex()!.getWorkspaceSize());
  }

  public getIndexWorkspace(): SpatialIndexWorkspace | null {
    return this.indexWorkspace;
  }
}
