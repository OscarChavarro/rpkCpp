/**
Generate and trace a local line
*/

import { RayExclusionSet } from "../../environment/geometry/elements/RayExclusionSet";
import { CoordinateSystem } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/CoordinateSystem";
import { Numeric } from "../../skin/Numeric";
import { Ray } from "vitral/dist/vsdk/toolkit/environment/geometry/element/Ray";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
import { SceneQuery } from "../../scene/SceneQuery";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";
import { Patch } from "../../environment/geometry/elements/Patch";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { StochasticRelaxation } from "./StochasticRelaxation";
import { Double } from "vitral/dist/java/lang/Double";

export class Localline {
  private constructor() {
  }

  /**
Creates a coordinate system on the patch P with Z direction along the normal
*/
  private static patchCoordSys(patch: Patch, coord: CoordinateSystem): void {
    const z = patch.getNormal();
    let x = new Vector3Dd();
    x = patch.vertex[1]!.point.subtract(patch.vertex[0]!.point);
    x = x.normalized(Numeric.EPSILON);
    let y = new Vector3Dd();
    y = z.crossProduct(x);

    coord.setXAxis(x);
    coord.setYAxis(y);
    coord.setZAxis(z);
  }

  /**
Constructs a ray with uniformly chosen origin on patch and cosine distributed
direction with respect to patch normal. Origin and direction are uniquely determined
by the 4-dimensional sample vector xi
*/
  public static mcrGenerateLocalLine(patch: Patch, xi: number[]): Ray {
    let ray = new Ray();
    const pdf = [0.0];

    if (patch !== Localline.previousPatch) {
      Localline.patchCoordSys(patch, Localline.coordSys);
      Localline.previousPatch = patch;
    }

    ray = ray.withOrigin(patch.uniformPoint(xi[0]!, xi[1]!) ?? ray.getOrigin());
    ray = ray.withDirection(Localline.coordSys.sampleHemisphereCosTheta(xi[2]!, xi[3]!, pdf));

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
    const distance = [Double.MAX_VALUE];

    const exclusions = new RayExclusionSet(P, P.twin);
    const hit = sceneQuery.intersect(
      ray,
      Numeric.EPSILON < P.getTolerance() ? Numeric.EPSILON : P.getTolerance(),
      distance,
      RayHitFlag.FRONT | RayHitFlag.POINT,
      hitStore,
      exclusions,
      sceneWorldSceneQueryWorkspace
    );
    Localline.someFeedback();

    return hit;
  }

  private static previousPatch: Patch | null = null;
  private static readonly coordSys = new CoordinateSystem();
}
