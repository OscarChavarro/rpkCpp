import { CoordinateSystem } from "../../common/linealAlgebra/CoordinateSystem";
import { VisibilityQueryRecorder } from "../../scene/VisibilityQueryRecorder";
import { RayExclusionSet } from "../../environment/geometry/elements/RayExclusionSet";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Statistics } from "../../common/statistics/Statistics";
import { PhongBidirectionalScatteringDistributionFunction } from "../../material/PhongBidirectionalScatteringDistributionFunction";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
import { Camera } from "../../scene/Camera";
import { SceneQuery } from "../../scene/SceneQuery";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";
import { Patch } from "../../environment/geometry/elements/Patch";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { SimpleRaytracingPathNode } from "./SimpleRaytracingPathNode";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";
import { Float } from "vitral/dist/java/lang/Float";

export class RayTools {
  private static pathFrontHitFlags(): number {
    return RayHitFlag.FRONT | RayHitFlag.POINT | RayHitFlag.MATERIAL;
  }

  private static traceWorld(
    sceneQuery: SceneQuery,
    sceneWorldSceneQueryWorkspace: SceneQueryWorkspace,
    ray: Ray,
    patch: Patch | null,
    flags: number,
    extraPatch: Patch | null,
    hitStore: RayHit | null
  ): RayHit | null {
    const myHitStore = new RayHit();
    const dist = [Float.MAX_VALUE];
    const localHitStore = hitStore === null ? myHitStore : hitStore;

    const exclusions = new RayExclusionSet(patch, patch !== null ? patch.twin : null, extraPatch);
    const result = sceneQuery.intersect(ray, 0.0, dist, flags, localHitStore, exclusions, sceneWorldSceneQueryWorkspace);
    const recorder = sceneWorldSceneQueryWorkspace.getRecorder();
    if (recorder !== null) {
      recorder.recordQuery(
        VisibilityQueryRecorder.RAYTRACER_CLOSEST_HIT, ray, 0.0, Float.MAX_VALUE,
        flags, exclusions, result, dist[0]!);
    }

    if (result !== null) {
      const frame = result.getShadingFrame();
      result.setShadingFrame(frame);
    }

    return result;
  }

  public static findRayIntersection(
    sceneQuery: SceneQuery,
    sceneWorldSceneQueryWorkspace: SceneQueryWorkspace,
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

    let newHit = RayTools.traceWorld(sceneQuery, sceneWorldSceneQueryWorkspace, ray, patch, hitFlags, null, hitStore);
    Statistics.instance().rayTracer.rayCount++;

    if (newHit !== null
      && (newHit.getFlags() & RayHitFlag.BACK) !== 0
      && newHit.getPatch() !== null
      && newHit.getPatch()!.material !== null
      && newHit.getPatch()!.material!.getBsdf() !== currentBsdf) {
      newHit = RayTools.traceWorld(sceneQuery, sceneWorldSceneQueryWorkspace, ray, patch, hitFlags, newHit.getPatch(), hitStore);
      Statistics.instance().rayTracer.rayCount++;
    }

    return newHit;
  }

  public static pathNodesVisible(
    sceneQuery: SceneQuery,
    sceneWorldSceneQueryWorkspace: SceneQueryWorkspace,
    node1: SimpleRaytracingPathNode,
    node2: SimpleRaytracingPathNode
  ): boolean {
    const patch1 = node1.m_hit.getPatch();
    const patch2 = node2.m_hit.getPatch();
    if (patch1 !== null && patch1 === patch2) {
      return false;
    }

    let dir = new Vector3Df();
    const ray = new Ray();
    const hitStore = new RayHit();
    const fDistance = [0.0];

    dir = node2.m_hit.getPoint().subtract(node1.m_hit.getPoint());

    const dist2 = dir.lengthSquared();
    let dist = globalThis.Math.sqrt(dist2);
    dir = dir.multiplyByInverse(dist, Numeric.EPSILON_FLOAT);

    dist = dist * (1.0 - VSDK.EPSILON);

    ray.position = node1.m_hit.getPoint();
    ray.direction = dir;

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
        fDistance[0] = Float.MAX_VALUE;
      }
      else {
        fDistance[0] = Math.fround(dist);
      }

      const queryMaximumDistance = fDistance[0]!;
      const exclusions = new RayExclusionSet(
        patch2,
        patch1,
        patch1 !== null ? patch1.twin : null
      );
      const hit = sceneQuery.intersect(
        ray,
        0.0,
        fDistance,
        RayHitFlag.FRONT | RayHitFlag.BACK | RayHitFlag.ANY,
        hitStore,
        exclusions,
        sceneWorldSceneQueryWorkspace
      );
      const recorder = sceneWorldSceneQueryWorkspace.getRecorder();
      if (recorder !== null) {
        recorder.recordQuery(
          VisibilityQueryRecorder.RAYTRACER_VISIBILITY, ray, 0.0, queryMaximumDistance,
          RayHitFlag.FRONT | RayHitFlag.BACK | RayHitFlag.ANY, exclusions, hit, fDistance[0]!);
      }
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
    sceneQuery: SceneQuery,
    sceneWorldSceneQueryWorkspace: SceneQueryWorkspace,
    eyeNode: SimpleRaytracingPathNode,
    node: SimpleRaytracingPathNode,
    pixX: number[] | null,
    pixY: number[] | null
  ): boolean {
    let dir = new Vector3Df();
    const ray = new Ray();
    const hitStore = new RayHit();
    const fDistance = [0.0];

    dir = node.m_hit.getPoint().subtract(eyeNode.m_hit.getPoint());

    const dist2 = dir.lengthSquared();
    let dist = globalThis.Math.sqrt(dist2);

    dir = dir.multiplyByInverse(dist, Numeric.EPSILON_FLOAT);

    const z = dir.dotProduct(camera.Z);

    let visible = false;
    if (z > 0.0) {
      const x = dir.dotProduct(camera.X);
      const xz = x / z;

      if (globalThis.Math.abs(xz) < camera.pixelWidthTangent) {
        const y = dir.dotProduct(camera.Y);
        const yz = y / z;

        if (globalThis.Math.abs(yz) < camera.pixelHeightTangent) {
          dist = dist * (1.0 - VSDK.EPSILON);

          ray.position = eyeNode.m_hit.getPoint();
          ray.direction = dir;

          const cosRayEye = dir.dotProduct(eyeNode.m_normal);
          const cosRayLight = -dir.dotProduct(node.m_normal);

          if (cosRayLight > 0.0 && cosRayEye > 0.0) {
            fDistance[0] = Math.fround(dist);
            const queryMaximumDistance = fDistance[0]!;
            const nodePatch = node.m_hit.getPatch();
            const eyePatch = eyeNode.m_hit.getPatch();
            const exclusions = new RayExclusionSet(
              nodePatch,
              eyePatch,
              eyePatch !== null ? eyePatch.twin : null
            );
            const hit = sceneQuery.intersect(
              ray,
              0.0,
              fDistance,
              RayHitFlag.FRONT | RayHitFlag.ANY,
              hitStore,
              exclusions,
              sceneWorldSceneQueryWorkspace
            );
            const recorder = sceneWorldSceneQueryWorkspace.getRecorder();
            if (recorder !== null) {
              recorder.recordQuery(
                VisibilityQueryRecorder.RAYTRACER_VISIBILITY, ray, 0.0, queryMaximumDistance,
                RayHitFlag.FRONT | RayHitFlag.ANY, exclusions, hit, fDistance[0]!);
            }
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
