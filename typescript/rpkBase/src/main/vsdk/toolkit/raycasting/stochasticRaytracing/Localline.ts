/**
Generate and trace a local line
*/

import { RayExclusionSet } from "../../environment/geometry/elements/RayExclusionSet";
import { VisibilityQueryRecorder } from "../../scene/VisibilityQueryRecorder";
import { CoordinateSystem } from "../../common/linealAlgebra/CoordinateSystem";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
import { SceneQuery } from "../../scene/SceneQuery";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";
import { Patch } from "../../environment/geometry/elements/Patch";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { StochasticRelaxation } from "./StochasticRelaxation";

export class Localline {
  private constructor() {
  }

  /**
Creates a coordinate system on the patch P with Z direction along the normal
*/
  private static patchCoordSys(patch: Patch, coord: CoordinateSystem): void {
    const z = patch.getNormal();
    const x = new Vector3D();
    x.subtraction(patch.vertex[1]!.point, patch.vertex[0]!.point);
    x.normalize(Numeric.EPSILON_FLOAT);
    const y = new Vector3D();
    y.crossProduct(z, x);

    coord.setX(x);
    coord.setY(y);
    coord.setZ(z);
  }

  /**
Constructs a ray with uniformly chosen origin on patch and cosine distributed
direction with respect to patch normal. Origin and direction are uniquely determined
by the 4-dimensional sample vector xi
*/
  public static mcrGenerateLocalLine(patch: Patch, xi: number[]): Ray {
    const ray = new Ray();
    const pdf = [0.0];

    if (patch !== Localline.previousPatch) {
      Localline.patchCoordSys(patch, Localline.coordSys);
      Localline.previousPatch = patch;
    }

    patch.uniformPoint(xi[0]!, xi[1]!, ray.position);
    ray.direction = Localline.coordSys.sampleHemisphereCosTheta(xi[2]!, xi[3]!, pdf);

    return ray;
  }

  /**
In order to let the user have the impression that the computations are proceeding
*/
  private static someFeedback(): void {
    if ((StochasticRelaxation.activeState().tracedRays + StochasticRelaxation.activeState().importanceTracedRays) % 1000 === 0) {
      process.stderr.write(".");
    }
  }

  /**
Determines nearest intersection point and patch
*/
  public static mcrShootRay(sceneQuery: SceneQuery, sceneWorldSceneQueryWorkspace: SceneQueryWorkspace, P: Patch, ray: Ray, hitStore: RayHit): RayHit | null {
    const distance = [Numeric.HUGE_FLOAT_VALUE];

    const exclusions = new RayExclusionSet(P, P.twin);
    const hit = sceneQuery.intersect(
      ray,
      Numeric.EPSILON_FLOAT < P.getTolerance() ? Numeric.EPSILON_FLOAT : P.getTolerance(),
      distance,
      RayHitFlag.FRONT | RayHitFlag.POINT,
      hitStore,
      exclusions,
      sceneWorldSceneQueryWorkspace
    );
    const recorder = sceneWorldSceneQueryWorkspace.getRecorder();
    if (recorder !== null) {
      recorder.recordQuery(
        VisibilityQueryRecorder.STOCHASTIC_SHOOTING, ray,
        Numeric.EPSILON_FLOAT < P.getTolerance() ? Numeric.EPSILON_FLOAT : P.getTolerance(),
        Numeric.HUGE_FLOAT_VALUE, RayHitFlag.FRONT | RayHitFlag.POINT, exclusions, hit, distance[0]!);
    }
    Localline.someFeedback();

    return hit;
  }

  private static previousPatch: Patch | null = null;
  private static readonly coordSys = new CoordinateSystem();
}
