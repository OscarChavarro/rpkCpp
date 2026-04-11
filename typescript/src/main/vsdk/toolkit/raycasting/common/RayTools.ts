import { CoordinateSystem } from "../../common/linealAlgebra/CoordinateSystem";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { PhongBidirectionalScatteringDistributionFunction } from "../../material/PhongBidirectionalScatteringDistributionFunction";
import { RayHitFlag } from "../../material/RayHitFlag";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Patch } from "../../skin/Patch";
import { RayHit } from "../../skin/RayHit";
import { SimpleRaytracingPathNode } from "./SimpleRaytracingPathNode";

export class RayTools {
  private static pathFrontHitFlags(): number {
    return RayHitFlag.FRONT | RayHitFlag.POINT | RayHitFlag.MATERIAL;
  }

  private static traceWorld(
    sceneWorldVoxelGrid: VoxelGrid,
    ray: Ray,
    patch: Patch | null,
    flags: number,
    extraPatch: Patch | null,
    hitStore: RayHit | null
  ): RayHit | null {
    const myHitStore = new RayHit();
    const dist = [Numeric.HUGE_FLOAT_VALUE];
    const localHitStore = hitStore === null ? myHitStore : hitStore;

    Patch.dontIntersect3(patch, patch !== null ? patch.twin : null, extraPatch);
    const result = sceneWorldVoxelGrid.gridIntersect(ray, 0.0, dist, flags, localHitStore);

    if (result !== null) {
      const frame = result.getShadingFrame();
      result.setShadingFrame(frame);
    }
    Patch.dontIntersect0();

    return result;
  }

  public static findRayIntersection(
    sceneWorldVoxelGrid: VoxelGrid,
    ray: Ray,
    patch: Patch | null,
    currentBsdf: PhongBidirectionalScatteringDistributionFunction | null,
    hitStore: RayHit | null
  ): RayHit | null {
    let hitFlags: number;

    if (currentBsdf === null) {
      hitFlags = RayTools.pathFrontHitFlags();
    }
    else {
      hitFlags = RayTools.pathFrontHitFlags() | RayHitFlag.BACK;
    }

    let newHit = RayTools.traceWorld(sceneWorldVoxelGrid, ray, patch, hitFlags, null, hitStore);
    Statistics.instance().rayTracer.rayCount++;

    if (newHit !== null
      && (newHit.getFlags() & RayHitFlag.BACK) !== 0
      && newHit.getPatch() !== null
      && newHit.getPatch()!.material !== null
      && newHit.getPatch()!.material!.getBsdf() !== currentBsdf) {
      newHit = RayTools.traceWorld(sceneWorldVoxelGrid, ray, patch, hitFlags, newHit.getPatch(), hitStore);
      Statistics.instance().rayTracer.rayCount++;
    }

    return newHit;
  }

  public static pathNodesVisible(
    sceneWorldVoxelGrid: VoxelGrid,
    node1: SimpleRaytracingPathNode,
    node2: SimpleRaytracingPathNode
  ): boolean {
    const patch1 = node1.m_hit.getPatch();
    const patch2 = node2.m_hit.getPatch();
    if (patch1 !== null && patch1 === patch2) {
      return false;
    }

    const dir = new Vector3D();
    const ray = new Ray();
    const hitStore = new RayHit();
    const fDistance = [0.0];

    dir.subtraction(node2.m_hit.getPoint(), node1.m_hit.getPoint());

    const dist2 = dir.norm2();
    let dist = globalThis.Math.sqrt(dist2);
    dir.inverseScaledCopy(dist, dir, Numeric.EPSILON_FLOAT);

    dist = dist * (1.0 - Numeric.EPSILON);

    ray.position = node1.m_hit.getPoint();
    ray.direction.copy(dir);

    const cosRay1 = dir.dotProduct(node1.m_normal);
    const cosRay2 = -dir.dotProduct(node2.m_normal);

    let doTest = false;

    if (cosRay1 > 0.0) {
      if (cosRay2 > 0.0) {
        doTest = true;
      }
      else if (node1.m_inBsdf === node2.m_outBsdf) {
        doTest = true;
      }
    }
    else if (cosRay2 > 0.0) {
      if (node1.m_outBsdf === node2.m_inBsdf) {
        doTest = true;
      }
    }
    else if (node1.m_outBsdf === node2.m_outBsdf) {
      doTest = true;
    }

    let visible: boolean;
    if (doTest) {
      if (patch2 !== null && patch2.hasZeroVertices()) {
        fDistance[0] = Numeric.HUGE_FLOAT_VALUE;
      }
      else {
        fDistance[0] = dist;
      }

      Patch.dontIntersect3(
        patch2,
        patch1,
        patch1 !== null ? patch1.twin : null
      );
      const hit = sceneWorldVoxelGrid.gridIntersect(
        ray,
        0.0,
        fDistance,
        RayHitFlag.FRONT | RayHitFlag.BACK | RayHitFlag.ANY,
        hitStore
      );
      Patch.dontIntersect0();
      visible = hit === null;

      Statistics.instance().rayTracer.rayCount++;
    }
    else {
      visible = false;
    }

    return visible;
  }

  public static eyeNodeVisible(
    camera: Camera,
    sceneWorldVoxelGrid: VoxelGrid,
    eyeNode: SimpleRaytracingPathNode,
    node: SimpleRaytracingPathNode,
    pixX: number[] | null,
    pixY: number[] | null
  ): boolean {
    const dir = new Vector3D();
    const ray = new Ray();
    const hitStore = new RayHit();
    const fDistance = [0.0];

    dir.subtraction(node.m_hit.getPoint(), eyeNode.m_hit.getPoint());

    const dist2 = dir.norm2();
    let dist = globalThis.Math.sqrt(dist2);

    dir.inverseScaledCopy(dist, dir, Numeric.EPSILON_FLOAT);

    const z = dir.dotProduct(camera.Z);

    let visible = false;
    if (z > 0.0) {
      const x = dir.dotProduct(camera.X);
      const xz = x / z;

      if (globalThis.Math.abs(xz) < camera.pixelWidthTangent) {
        const y = dir.dotProduct(camera.Y);
        const yz = y / z;

        if (globalThis.Math.abs(yz) < camera.pixelHeightTangent) {
          dist = dist * (1.0 - Numeric.EPSILON);

          ray.position = eyeNode.m_hit.getPoint();
          ray.direction.copy(dir);

          const cosRayEye = dir.dotProduct(eyeNode.m_normal);
          const cosRayLight = -dir.dotProduct(node.m_normal);

          if (cosRayLight > 0.0 && cosRayEye > 0.0) {
            fDistance[0] = dist;
            const nodePatch = node.m_hit.getPatch();
            const eyePatch = eyeNode.m_hit.getPatch();
            Patch.dontIntersect3(
              nodePatch,
              eyePatch,
              eyePatch !== null ? eyePatch.twin : null
            );
            const hit = sceneWorldVoxelGrid.gridIntersect(
              ray,
              0.0,
              fDistance,
              RayHitFlag.FRONT | RayHitFlag.ANY,
              hitStore
            );
            Patch.dontIntersect0();
            visible = hit === null;

            if (visible) {
              if (pixX !== null && pixX.length > 0) {
                pixX[0] = xz;
              }
              if (pixY !== null && pixY.length > 0) {
                pixY[0] = yz;
              }
            }
          }
        }
      }
    }

    return visible;
  }
}
