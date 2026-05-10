/**
Generic stochastic Jacobi iteration (local lines)
TODO: combined radiance / importance propagation
TODO: hierarchical refinement for importance propagation
TODO: re-incorporate the rejection sampling technique for
sampling positions on shooters with higher order radiosity approximation
(lower variance)
TODO: lines and line bundles.
*/

import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Error as VsdkError } from "../../common/Error";
import { RenderOptions } from "../../common/RenderOptions";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { RayHitFlag } from "../../skin/RayHitFlag";
import { Niederreiter } from "../../numericalAnalysis/quasiMonteCarlo/Niederreiter";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Element } from "../../skin/Element";
import { Patch } from "../../skin/Patch";
import { RayHit } from "../../skin/RayHit";
import { Basismcrad } from "./Basismcrad";
import { Ccr } from "./Ccr";
import { Coefficientsmcrad } from "./Coefficientsmcrad";
import { ElementHierarchyState } from "./ElementHierarchyState";
import { Hierarchy } from "./Hierarchy";
import { HierarchyClusteringMode } from "./HierarchyClusteringMode";
import { Link } from "./Link";
import { Localline } from "./Localline";
import { McradP } from "./McradP";
import { StochasticRadiosityBasisState } from "./StochasticRadiosityBasisState";
import { StochasticRadiosityElement } from "./StochasticRadiosityElement";
import { StochasticRaytracingApproximation } from "./StochasticRaytracingApproximation";
import { StochasticRelaxation } from "./StochasticRelaxation";

export class StochasticJacobi {
  private constructor() {
  }

  private static getRadianceCallback: ((elem: StochasticRadiosityElement) => ColorRgb[] | null) | null = null;
  private static getImportanceCallback: ((elem: StochasticRadiosityElement) => number) | null = null;
  private static reflectCallback: ((elem: StochasticRadiosityElement, w: number) => void) | null = null;
  private static useControlVariate = 0;
  private static numberOfRaysToShoot = 0;
  private static sumOfProbabilities = 0.0;

  private static stochasticJacobiInitGlobals(
    numberOfRays: number,
    getRadianceCallBack: ((elem: StochasticRadiosityElement) => ColorRgb[] | null) | null,
    getImportanceCallBack: ((elem: StochasticRadiosityElement) => number) | null,
    updateCallBack: ((elem: StochasticRadiosityElement, w: number) => void) | null
  ): void {
    StochasticJacobi.numberOfRaysToShoot = numberOfRays;
    StochasticJacobi.getRadianceCallback = getRadianceCallBack;
    StochasticJacobi.getImportanceCallback = getImportanceCallBack;
    StochasticJacobi.reflectCallback = updateCallBack;
    StochasticJacobi.useControlVariate =
      (StochasticRelaxation.activeState().constantControlVariate !== 0 && getRadianceCallBack !== null) ? 1 : 0;

    if (StochasticJacobi.getRadianceCallback !== null) {
      StochasticRelaxation.activeState().prevTracedRays = StochasticRelaxation.activeState().tracedRays;
    }
    if (StochasticJacobi.getImportanceCallback !== null) {
      StochasticRelaxation.activeState().prevImportanceTracedRays = StochasticRelaxation.activeState().importanceTracedRays;
    }
  }

  private static stochasticJacobiPrintMessage(nrRays: number): void {
    process.stderr.write(
      `${StochasticRelaxation.activeState().bidirectionalTransfers !== 0 ? "Bi" : "Uni"}-directional `
    );
    if (StochasticJacobi.getRadianceCallback !== null && StochasticJacobi.getImportanceCallback !== null) {
      process.stderr.write("combined ");
    }
    if (StochasticJacobi.getRadianceCallback !== null) {
      process.stderr.write(
        `${StochasticRelaxation.activeState().importanceDriven !== 0 ? "importance-driven " : ""} radiance `
      );
    }
    if (StochasticJacobi.getRadianceCallback !== null && StochasticJacobi.getImportanceCallback !== null) {
      process.stderr.write("and ");
    }
    if (StochasticJacobi.getImportanceCallback !== null) {
      process.stderr.write(
        `${StochasticRelaxation.activeState().radianceDriven !== 0 ? "radiance-driven " : ""} importance `
      );
    }
    process.stderr.write("propagation");
    if (StochasticJacobi.useControlVariate !== 0) {
      process.stderr.write("using a constant control variate ");
    }
    process.stderr.write(`(${nrRays} rays):\n`);
  }

  /**
Compute (un-normalised) stochasticJacobiProbability of shooting a ray from elem
*/
  private static stochasticJacobiProbability(elem: StochasticRadiosityElement): number {
    let prob = 0.0;

    if (StochasticJacobi.getRadianceCallback !== null) {
      const callbackRadiance = StochasticJacobi.getRadianceCallback(elem)!;
      const radiance = new ColorRgb();
      radiance.set(callbackRadiance[0].r, callbackRadiance[0].g, callbackRadiance[0].b);
      if (StochasticRelaxation.activeState().constantControlVariate !== 0) {
        radiance.subtract(radiance, StochasticRelaxation.activeState().controlRadiance);
      }
      prob = elem.area * radiance.sumAbsComponents();
      if (StochasticRelaxation.activeState().importanceDriven !== 0) {
        const w = elem.importance - elem.sourceImportance;
        prob *= (w > 0.0) ? w : 0.0;
      }
    }

    if (StochasticJacobi.getImportanceCallback !== null && StochasticRelaxation.activeState().importanceDriven !== 0) {
      let prob2 = elem.area * globalThis.Math.abs(StochasticJacobi.getImportanceCallback(elem)) *
        StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(elem);

      if (StochasticRelaxation.activeState().radianceDriven !== 0) {
        const receivedRadiance = new ColorRgb();
        receivedRadiance.subtract(elem.radiance![0], elem.sourceRad);
        prob2 *= receivedRadiance.sumAbsComponents();
      }

      const approximation =
        (StochasticRelaxation.activeState().approximationOrderType
          ?? StochasticRaytracingApproximation.CONSTANT) as number;
      prob = prob * StochasticRadiosityBasisState.activeState().approxDesc[approximation].basis_size + prob2;
    }

    return prob;
  }

  /**
clear accumulators of all kinds of sample weights and contributions
*/
  private static stochasticJacobiElementClearAccumulators(elem: StochasticRadiosityElement): void {
    if (StochasticJacobi.getRadianceCallback !== null) {
      Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
    }
    if (StochasticJacobi.getImportanceCallback !== null) {
      elem.receivedImportance = 0.0;
    }
  }

  /**
Clears received radiance and importance and accumulates the un-normalized
sampling probabilities at leaf elements
*/
  private static stochasticJacobiElementSetup(element: Element): void {
    const stochasticRadiosityElement = element as StochasticRadiosityElement;

    if (stochasticRadiosityElement === null) {
      return;
    }

    stochasticRadiosityElement.samplingProbability = 0.0;
    if (!stochasticRadiosityElement.traverseAllChildren((child) => StochasticJacobi.stochasticJacobiElementSetup(child))) {
      stochasticRadiosityElement.samplingProbability = StochasticJacobi.stochasticJacobiProbability(stochasticRadiosityElement);
      StochasticJacobi.sumOfProbabilities += stochasticRadiosityElement.samplingProbability;
    }
    if (stochasticRadiosityElement.parent !== null) {
      (stochasticRadiosityElement.parent as StochasticRadiosityElement).samplingProbability +=
        stochasticRadiosityElement.samplingProbability;
    }

    StochasticJacobi.stochasticJacobiElementClearAccumulators(stochasticRadiosityElement);
  }

  /**
Returns true if success, that is: sum of sampling probabilities is nonzero
*/
  private static stochasticJacobiSetup(scenePatches: ArrayList<Patch>): boolean {
    StochasticRelaxation.activeState().controlRadiance.clear();
    if (StochasticJacobi.useControlVariate !== 0) {
      StochasticRelaxation.activeState().controlRadiance =
        Ccr.determineControlRadiosity(
          (elem) => StochasticJacobi.getRadianceCallback!(elem),
          null,
          scenePatches
        );
    }

    StochasticJacobi.sumOfProbabilities = 0.0;
    StochasticJacobi.stochasticJacobiElementSetup(ElementHierarchyState.activeState().topCluster as Element);

    if (StochasticJacobi.sumOfProbabilities < Numeric.EPSILON * Numeric.EPSILON) {
      VsdkError.warning("Iteration", "No sources");
      return false;
    }
    return true;
  }

  /**
Returns radiance to be propagated from the given location of the element
*/
  private static stochasticJacobiGetSourceRadiance(src: StochasticRadiosityElement, us: number, vs: number): ColorRgb {
    const srcRad = StochasticJacobi.getRadianceCallback!(src) as ColorRgb[];
    return Basismcrad.colorAtUv(src.basis!, srcRad, us, vs);
  }

  private static stochasticJacobiPropagateRadianceToSurface(
    rcv: StochasticRadiosityElement,
    ur: number,
    vr: number,
    rayPower: ColorRgb,
    src: StochasticRadiosityElement,
    fraction: number,
    weight: number
  ): void {
    if (src === null || weight < 0) {
      // Keep parameters used in C++ signature.
    }
    for (let i = 0; i < rcv.basis!.size; i++) {
      const dual = rcv.basis!.dualFunction![i](ur, vr) / rcv.area;
      const w = dual * fraction / StochasticJacobi.numberOfRaysToShoot;
      rcv.receivedRadiance![i].addScaled(rcv.receivedRadiance![i], w, rayPower);
    }
  }

  private static stochasticJacobiPropagateRadianceToClusterIsotropic(
    cluster: StochasticRadiosityElement,
    rayPower: ColorRgb,
    src: StochasticRadiosityElement,
    fraction: number,
    weight: number
  ): void {
    if (src === null || weight < 0) {
      // Keep parameters used in C++ signature.
    }
    const w = fraction / cluster.area / StochasticJacobi.numberOfRaysToShoot;
    cluster.receivedRadiance![0].addScaled(cluster.receivedRadiance![0], w, rayPower);
  }

  /**
Note: Not considering the MAX_HIERARCHY_DEPTH limit.
*/
  private static stochasticJacobiPropagateRadianceClusterRecursive(
    currentElement: StochasticRadiosityElement,
    rayPower: ColorRgb,
    ray: Ray,
    dir: number,
    projectedArea: number,
    fraction: number
  ): void {
    if (currentElement !== null && !currentElement.isCluster()) {
      const c = -dir * currentElement.patch!.normal.dotProduct(ray.direction);
      if (c > 0.0) {
        const aFraction = fraction * (c * currentElement.area / projectedArea);
        const w = aFraction / currentElement.area / StochasticJacobi.numberOfRaysToShoot;
        currentElement.receivedRadiance![0].addScaled(currentElement.receivedRadiance![0], w, rayPower);
      }
    }
    else {
      for (
        let i = 0;
        currentElement !== null && currentElement.irregularSubElements !== null && i < currentElement.irregularSubElements.length;
        i++
      ) {
        StochasticJacobi.stochasticJacobiPropagateRadianceClusterRecursive(
          currentElement.irregularSubElements[i] as StochasticRadiosityElement,
          rayPower,
          ray,
          dir,
          projectedArea,
          fraction
        );
      }
    }
  }

  private static stochasticJacobiPropagateRadianceToClusterOriented(
    cluster: StochasticRadiosityElement,
    rayPower: ColorRgb,
    ray: Ray,
    dir: number,
    src: StochasticRadiosityElement,
    projectedArea: number,
    fraction: number,
    weight: number
  ): void {
    if (src === null || weight < 0) {
      // Keep parameters used in C++ signature.
    }
    StochasticJacobi.stochasticJacobiPropagateRadianceClusterRecursive(cluster, rayPower, ray, dir, projectedArea, fraction);
  }

  /**
Note: Not considering the MAX_HIERARCHY_DEPTH limit.
*/
  private static stochasticJacobiReceiverProjectedAreaRecursive(
    currentElement: StochasticRadiosityElement,
    ray: Ray,
    dir: number,
    area: number[]
  ): void {
    if (currentElement !== null && !currentElement.isCluster()) {
      const c = -dir * currentElement.patch!.normal.dotProduct(ray.direction);
      if (c > 0.0) {
        area[0] += c * currentElement.area;
      }
    }
    else {
      for (
        let i = 0;
        currentElement !== null && currentElement.irregularSubElements !== null && i < currentElement.irregularSubElements.length;
        i++
      ) {
        StochasticJacobi.stochasticJacobiReceiverProjectedAreaRecursive(
          currentElement.irregularSubElements[i] as StochasticRadiosityElement,
          ray,
          dir,
          area
        );
      }
    }
  }

  private static stochasticJacobiReceiverProjectedArea(cluster: StochasticRadiosityElement, ray: Ray, dir: number): number {
    const area = [0.0];
    StochasticJacobi.stochasticJacobiReceiverProjectedAreaRecursive(cluster, ray, dir, area);
    return area[0];
  }

  /**
Transfer radiance from src to rcv.
*/
  private static stochasticJacobiPropagateRadiance(
    src: StochasticRadiosityElement,
    us: number,
    vs: number,
    rcv: StochasticRadiosityElement,
    ur: number,
    vr: number,
    srcProb: number,
    rcvProb: number,
    ray: Ray,
    dir: number
  ): void {
    const rayPower = new ColorRgb();
    let area: number;
    const weight = StochasticJacobi.sumOfProbabilities / srcProb;
    const fraction = srcProb / (srcProb + rcvProb);

    if (srcProb < Numeric.EPSILON * Numeric.EPSILON || fraction < Numeric.EPSILON) {
      return;
    }

    const radiance = StochasticJacobi.stochasticJacobiGetSourceRadiance(src, us, vs);
    if (StochasticRelaxation.activeState().constantControlVariate !== 0) {
      radiance.subtract(radiance, StochasticRelaxation.activeState().controlRadiance);
    }
    rayPower.scaledCopy(weight, radiance);

    if (!rcv.isCluster()) {
      StochasticJacobi.stochasticJacobiPropagateRadianceToSurface(rcv, ur, vr, rayPower, src, fraction, weight);
    }
    else {
      switch (ElementHierarchyState.activeState().clustering) {
        case HierarchyClusteringMode.NO_CLUSTERING:
          VsdkError.fatal(
            -1,
            "Propagate",
            "Hierarchy::hierarchyRefine() returns cluster although clustering is disabled.\n"
          );
          break;
        case HierarchyClusteringMode.ISOTROPIC_CLUSTERING:
          StochasticJacobi.stochasticJacobiPropagateRadianceToClusterIsotropic(rcv, rayPower, src, fraction, weight);
          break;
        case HierarchyClusteringMode.ORIENTED_CLUSTERING:
          area = StochasticJacobi.stochasticJacobiReceiverProjectedArea(rcv, ray, dir);
          if (area > Numeric.EPSILON) {
            StochasticJacobi.stochasticJacobiPropagateRadianceToClusterOriented(
              rcv,
              rayPower,
              ray,
              dir,
              src,
              area,
              fraction,
              weight
            );
          }
          break;
        default:
          VsdkError.fatal(
            -1,
            "Propagate",
            "Invalid clustering mode %d\n",
            (ElementHierarchyState.activeState().clustering as number)
          );
          break;
      }
    }
  }

  /**
Idem but for importance
*/
  private static stochasticJacobiPropagateImportance(
    src: StochasticRadiosityElement,
    us: number,
    vs: number,
    rcv: StochasticRadiosityElement,
    ur: number,
    vr: number,
    srcProb: number,
    rcvProb: number,
    ray: Ray,
    dir: number
  ): void {
    if (ray === null || us < -1 || vs < -1 || ur < -1 || vr < -1 || dir < -2) {
      // Keep parameters used in C++ signature.
    }

    const w = StochasticJacobi.sumOfProbabilities / (srcProb + rcvProb) / rcv.area / StochasticJacobi.numberOfRaysToShoot;
    rcv.receivedImportance += w * StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(src)
      * StochasticJacobi.getImportanceCallback!(src);

    if (ElementHierarchyState.activeState().do_h_meshing !== 0 ||
      ElementHierarchyState.activeState().clustering !== HierarchyClusteringMode.NO_CLUSTERING) {
      VsdkError.fatal(
        -1,
        "Propagate",
        "Importance propagation not implemented in combination with hierarchical refinement"
      );
    }
  }

  /**
Src is the leaf element containing the point from which to propagate
*/
  private static stochasticJacobiRefineAndPropagateRadiance(
    src: StochasticRadiosityElement,
    us: number,
    vs: number,
    P: StochasticRadiosityElement,
    up: number,
    vp: number,
    Q: StochasticRadiosityElement,
    uq: number,
    vq: number,
    srcProb: number,
    rcvProb: number,
    ray: Ray,
    dir: number,
    renderOptions: RenderOptions
  ): void {
    let link = Hierarchy.topLink(Q, P);
    const rcvU = [uq];
    const rcvV = [vq];
    const srcU = [up];
    const srcV = [vp];
    link = Hierarchy.hierarchyRefine(
      link,
      Q,
      rcvU,
      rcvV,
      P,
      srcU,
      srcV,
      ElementHierarchyState.activeState().oracle!,
      renderOptions
    );

    StochasticJacobi.stochasticJacobiPropagateRadiance(src, us, vs, link.rcv as StochasticRadiosityElement,
      rcvU[0], rcvV[0], srcProb, rcvProb, ray, dir);
  }

  private static stochasticJacobiRefineAndPropagateImportance(
    P: StochasticRadiosityElement,
    up: number,
    vp: number,
    Q: StochasticRadiosityElement,
    uq: number,
    vq: number,
    srcProb: number,
    rcvProb: number,
    ray: Ray,
    dir: number
  ): void {
    StochasticJacobi.stochasticJacobiPropagateImportance(P, up, vp, Q, uq, vq, srcProb, rcvProb, ray, dir);
  }

  /**
Ray is a ray connecting the positions with given (u,v) parameters
*/
  private static stochasticJacobiRefineAndPropagate(
    P: StochasticRadiosityElement,
    up: number,
    vp: number,
    Q: StochasticRadiosityElement,
    uq: number,
    vq: number,
    ray: Ray,
    renderOptions: RenderOptions
  ): void {
    let srcProb: number;
    const us = [up];
    const vs = [vp];
    const src = StochasticRadiosityElement.stochasticRadiosityElementRegularLeafElementAtPoint(P, us, vs);
    srcProb = src.samplingProbability / src.area;

    if (StochasticRelaxation.activeState().bidirectionalTransfers !== 0) {
      let rcvProb: number;
      const ur = [uq];
      const vr = [vq];
      const rcv = StochasticRadiosityElement.stochasticRadiosityElementRegularLeafElementAtPoint(Q, ur, vr);
      rcvProb = rcv.samplingProbability / rcv.area;

      if (StochasticJacobi.getRadianceCallback !== null) {
        StochasticJacobi.stochasticJacobiRefineAndPropagateRadiance(
          src, us[0], vs[0], P, up, vp, Q, uq, vq, srcProb, rcvProb, ray, +1.0, renderOptions
        );
        StochasticJacobi.stochasticJacobiRefineAndPropagateRadiance(
          rcv, ur[0], vr[0], Q, uq, vq, P, up, vp, rcvProb, srcProb, ray, -1.0, renderOptions
        );
      }
      if (StochasticJacobi.getImportanceCallback !== null) {
        StochasticJacobi.stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, srcProb, rcvProb, ray, +1.0);
        StochasticJacobi.stochasticJacobiRefineAndPropagateImportance(Q, uq, vq, P, up, vp, rcvProb, srcProb, ray, -1.0);
      }
    }
    else {
      if (StochasticJacobi.getRadianceCallback !== null) {
        StochasticJacobi.stochasticJacobiRefineAndPropagateRadiance(
          src, us[0], vs[0], P, up, vp, Q, uq, vq, srcProb, 0.0, ray, +1.0, renderOptions
        );
      }
      if (StochasticJacobi.getImportanceCallback !== null) {
        StochasticJacobi.stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, srcProb, 0.0, ray, +1.0);
      }
    }
  }

  private static stochasticJacobiNextSample(
    elem: StochasticRadiosityElement,
    nMostSignificantBit: number,
    mostSignificantBit1: bigint,
    rMostSignificantBit2: bigint,
    zeta: number[]
  ): number[] {
    const rayIndex = [BigInt(StochasticJacobi.getRadianceCallback !== null ? elem.rayIndex : elem.importanceRayIndex)];
    const xi = Niederreiter.NextNiedInRange(
      rayIndex,
      +1,
      nMostSignificantBit,
      mostSignificantBit1,
      rMostSignificantBit2
    ) ?? [0n, 0n, 0n, 0n];

    rayIndex[0] += 1n;
    if (StochasticJacobi.getRadianceCallback !== null) {
      elem.rayIndex = Number(rayIndex[0]);
    }
    else {
      elem.importanceRayIndex = Number(rayIndex[0]);
    }

    let u = (xi[0] & ~3n) | 1n;
    let v = (xi[1] & ~3n) | 1n;
    if (elem.numberOfVertices === 3) {
      const uu = [u];
      const vv = [v];
      Niederreiter.foldSample(uu, vv);
      u = uu[0];
      v = vv[0];
    }
    zeta[0] = Number(u) * Niederreiter.RECIP;
    zeta[1] = Number(v) * Niederreiter.RECIP;
    zeta[2] = Number(xi[2]) * Niederreiter.RECIP;
    zeta[3] = Number(xi[3]) * Niederreiter.RECIP;
    return zeta;
  }

  /**
Determines uniform (u,v) parameters of hit point on hit patch
*/
  private static stochasticJacobiUniformHitCoordinates(hit: RayHit, uHit: number[], vHit: number[]): void {
    if ((hit.getFlags() & RayHitFlag.UV) !== 0) {
      uHit[0] = hit.getUv().u;
      vHit[0] = hit.getUv().v;
      if (hit.getPatch()!.jacobian !== null) {
        hit.getPatch()!.biLinearToUniform(uHit, vHit);
      }
    }
    else {
      const position: Vector3D = hit.getPoint();
      hit.getPatch()!.uniformUv(position, uHit, vHit);
    }

    if (uHit[0] < Numeric.EPSILON) {
      uHit[0] = Numeric.EPSILON;
    }
    if (vHit[0] < Numeric.EPSILON) {
      vHit[0] = Numeric.EPSILON;
    }
    if (uHit[0] > 1.0 - Numeric.EPSILON) {
      uHit[0] = 1.0 - Numeric.EPSILON;
    }
    if (vHit[0] > 1.0 - Numeric.EPSILON) {
      vHit[0] = 1.0 - Numeric.EPSILON;
    }
  }

  /**
Traces a local line from 'src' and propagates radiance and/or importance from P to
hit patch (and back for bidirectional transfers)
*/
  private static stochasticJacobiElementShootRay(
    sceneWorldVoxelGrid: VoxelGrid,
    src: StochasticRadiosityElement,
    nMostSignificantBit: number,
    mostSignificantBit1: bigint,
    rMostSignificantBit2: bigint,
    renderOptions: RenderOptions
  ): void {
    if (StochasticJacobi.getRadianceCallback !== null) {
      StochasticRelaxation.activeState().tracedRays++;
    }
    if (StochasticJacobi.getImportanceCallback !== null) {
      StochasticRelaxation.activeState().importanceTracedRays++;
    }

    const zeta = new Array<number>(4);
    const ray = Localline.mcrGenerateLocalLine(
      src.patch as Patch,
      StochasticJacobi.stochasticJacobiNextSample(src, nMostSignificantBit, mostSignificantBit1, rMostSignificantBit2, zeta)
    );

    const hitStore = new RayHit();
    const hit = Localline.mcrShootRay(sceneWorldVoxelGrid, src.patch as Patch, ray, hitStore);

    if (hit !== null) {
      const uHit = [0.0];
      const vHit = [0.0];
      StochasticJacobi.stochasticJacobiUniformHitCoordinates(hit, uHit, vHit);
      StochasticJacobi.stochasticJacobiRefineAndPropagate(
        McradP.topLevelStochasticRadiosityElement(src.patch as Patch),
        zeta[0],
        zeta[1],
        McradP.topLevelStochasticRadiosityElement(hit.getPatch() as Patch),
        uHit[0],
        vHit[0],
        ray,
        renderOptions
      );
    }
    else {
      StochasticRelaxation.activeState().numberOfMisses++;
    }
  }

  private static stochasticJacobiInitPushRayIndex(element: Element): void {
    const stochasticRadiosityElement = element as StochasticRadiosityElement;
    if (stochasticRadiosityElement === null) {
      return;
    }
    const parent = stochasticRadiosityElement.parent as StochasticRadiosityElement;
    if (parent !== null) {
      stochasticRadiosityElement.rayIndex = parent.rayIndex;
      stochasticRadiosityElement.importanceRayIndex = parent.importanceRayIndex;
    }
    stochasticRadiosityElement.traverseAllChildren((child) => StochasticJacobi.stochasticJacobiInitPushRayIndex(child));
  }

  /**
Determines nr of rays to shoot from element and shoots this number of rays
*/
  private static stochasticJacobiElementShootRays(
    sceneWorldVoxelGrid: VoxelGrid,
    element: StochasticRadiosityElement,
    raysThisElem: number,
    renderOptions: RenderOptions
  ): void {
    const sampleRange = [0];
    const mostSignificantBit1 = [0n];
    const rMostSignificantBit2 = [0n];

    StochasticRadiosityElement.stochasticRadiosityElementRange(
      element,
      sampleRange,
      mostSignificantBit1,
      rMostSignificantBit2
    );

    for (let i = 0; i < raysThisElem; i++) {
      StochasticJacobi.stochasticJacobiElementShootRay(
        sceneWorldVoxelGrid,
        element,
        sampleRange[0],
        mostSignificantBit1[0],
        rMostSignificantBit2[0],
        renderOptions
      );
    }

    if (element !== null && !element.isLeaf()) {
      element.traverseAllChildren((child) => StochasticJacobi.stochasticJacobiInitPushRayIndex(child));
    }
  }

  private static stochasticJacobiShootRaysRecursive(
    sceneWorldVoxelGrid: VoxelGrid,
    element: StochasticRadiosityElement,
    rnd: number,
    rayCount: number[],
    cumulative: number[],
    renderOptions: RenderOptions
  ): void {
    if (element.regularSubElements === null) {
      const p = element.samplingProbability / StochasticJacobi.sumOfProbabilities;
      const raysThisLeaf =
        globalThis.Math.floor((cumulative[0] + p) * StochasticJacobi.numberOfRaysToShoot + rnd) - rayCount[0];

      if (raysThisLeaf > 0) {
        StochasticJacobi.stochasticJacobiElementShootRays(sceneWorldVoxelGrid, element, raysThisLeaf, renderOptions);
      }

      cumulative[0] += p;
      rayCount[0] += raysThisLeaf;
    }
    else {
      for (let i = 0; i < 4; i++) {
        StochasticJacobi.stochasticJacobiShootRaysRecursive(
          sceneWorldVoxelGrid,
          element.regularSubElements[i] as StochasticRadiosityElement,
          rnd,
          rayCount,
          cumulative,
          renderOptions
        );
      }
    }
  }

  /**
Fire off rays from the leaf elements, propagate radiance/importance
*/
  private static stochasticJacobiShootRays(
    sceneWorldVoxelGrid: VoxelGrid,
    scenePatches: ArrayList<Patch>,
    renderOptions: RenderOptions
  ): void {
    const rnd = globalThis.Math.random();
    const rayCount = [0];
    const cumulative = [0.0];

    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      StochasticJacobi.stochasticJacobiShootRaysRecursive(
        sceneWorldVoxelGrid,
        McradP.topLevelStochasticRadiosityElement(scenePatches.get(i)),
        rnd,
        rayCount,
        cumulative,
        renderOptions
      );
    }

    process.stderr.write("\n");
  }

  /**
Converts received radiance and importance at a leaf element into a new
approximation of total and un-shot radiance and importance
*/
  private static stochasticJacobiUpdateElement(elem: StochasticRadiosityElement): void {
    if (StochasticJacobi.getRadianceCallback !== null) {
      if (StochasticJacobi.useControlVariate !== 0) {
        elem.receivedRadiance![0].add(elem.receivedRadiance![0], StochasticRelaxation.activeState().controlRadiance);
      }
      Coefficientsmcrad.stochasticRadiosityMultiplyCoefficients(elem.Rd, elem.receivedRadiance, elem.basis);
    }

    StochasticJacobi.reflectCallback!(elem, StochasticJacobi.numberOfRaysToShoot / StochasticJacobi.sumOfProbabilities);

    StochasticRelaxation.activeState().unShotFlux.addScaled(
      StochasticRelaxation.activeState().unShotFlux,
      globalThis.Math.PI * elem.area,
      elem.unShotRadiance![0]
    );
    StochasticRelaxation.activeState().totalFlux.addScaled(
      StochasticRelaxation.activeState().totalFlux,
      globalThis.Math.PI * elem.area,
      elem.radiance![0]
    );
    StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.addScaled(
      StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux,
      globalThis.Math.PI * elem.area * (elem.importance - elem.sourceImportance),
      elem.unShotRadiance![0]
    );
    StochasticRelaxation.activeState().unShotYmp += elem.area * globalThis.Math.abs(elem.unShotImportance);
    StochasticRelaxation.activeState().totalYmp += elem.area * elem.importance;
  }

  private static stochasticJacobiPush(parent: StochasticRadiosityElement, child: StochasticRadiosityElement): void {
    if (StochasticJacobi.getRadianceCallback !== null) {
      if (parent.isCluster() && !child.isCluster()) {
        const rad = new ColorRgb(parent.receivedRadiance![0].r, parent.receivedRadiance![0].g, parent.receivedRadiance![0].b);
        const Rd = child.Rd;
        rad.selfScalarProduct(Rd);
        StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(parent, child, [rad], child.receivedRadiance!);
      }
      else {
        StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(
          parent,
          child,
          parent.receivedRadiance!,
          child.receivedRadiance!
        );
      }
    }

    if (StochasticJacobi.getImportanceCallback !== null) {
      const parentImportance = [parent.receivedImportance];
      const childImportance = [child.receivedImportance];
      StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentImportance, childImportance);
      child.receivedImportance = childImportance[0];
    }
  }

  private static stochasticJacobiPull(parent: StochasticRadiosityElement, child: StochasticRadiosityElement): void {
    if (StochasticJacobi.getRadianceCallback !== null) {
      StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.radiance!, child.radiance!);
      StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.unShotRadiance!, child.unShotRadiance!);
    }
    if (StochasticJacobi.getImportanceCallback !== null) {
      const parentImportance = [parent.importance];
      const childImportance = [child.importance];
      StochasticRadiosityElement.stochasticRadiosityElementPullImportance(parent, child, parentImportance, childImportance);
      parent.importance = parentImportance[0];

      const parentUnShotImportance = [parent.unShotImportance];
      const childUnShotImportance = [child.unShotImportance];
      StochasticRadiosityElement.stochasticRadiosityElementPullImportance(
        parent,
        child,
        parentUnShotImportance,
        childUnShotImportance
      );
      parent.unShotImportance = parentUnShotImportance[0];
    }
  }

  /**
Clears everything to be pulled from children elements to zero
*/
  private static stochasticJacobiClearElement(parent: StochasticRadiosityElement): void {
    if (StochasticJacobi.getRadianceCallback !== null) {
      Coefficientsmcrad.stochasticRadiosityClearCoefficients(parent.radiance, parent.basis);
      Coefficientsmcrad.stochasticRadiosityClearCoefficients(parent.unShotRadiance, parent.basis);
    }
    if (StochasticJacobi.getImportanceCallback !== null) {
      parent.importance = 0.0;
      parent.unShotImportance = 0.0;
    }
  }

  private static stochasticJacobiPushUpdatePullChild(element: Element): void {
    const child = element as StochasticRadiosityElement;
    const parent = child.parent as StochasticRadiosityElement;
    StochasticJacobi.stochasticJacobiPush(parent, child);
    StochasticJacobi.stochasticJacobiPushUpdatePull(child);
    StochasticJacobi.stochasticJacobiPull(parent, child);
  }

  private static stochasticJacobiPushUpdatePull(element: Element): void {
    const stochasticRadiosityElement = element as StochasticRadiosityElement;
    if (stochasticRadiosityElement !== null && stochasticRadiosityElement.isLeaf()) {
      StochasticJacobi.stochasticJacobiUpdateElement(stochasticRadiosityElement);
    }
    else if (element !== null) {
      StochasticJacobi.stochasticJacobiClearElement(stochasticRadiosityElement);
      element.traverseAllChildren((child) => StochasticJacobi.stochasticJacobiPushUpdatePullChild(child));
    }
  }

  private static stochasticJacobiPullRdEdFromChild(element: Element): void {
    const child = element as StochasticRadiosityElement;
    const parent = child.parent as StochasticRadiosityElement;

    StochasticJacobi.stochasticJacobiPullRdEd(child);

    parent.Ed.addScaled(parent.Ed, child.area / parent.area, child.Ed);
    parent.Rd.addScaled(parent.Rd, child.area / parent.area, child.Rd);
    if (parent.isCluster()) {
      parent.Rd.setMonochrome(1.0);
    }
  }

  private static stochasticJacobiPullRdEd(element: StochasticRadiosityElement): void {
    if (element === null
      || element.isLeaf()
      || (!element.isCluster() && !StochasticRadiosityElement.stochasticRadiosityElementIsTextured(element))) {
      return;
    }

    element.Ed.clear();
    element.Rd.clear();
    element.traverseAllChildren((child) => StochasticJacobi.stochasticJacobiPullRdEdFromChild(child));
  }

  private static stochasticJacobiPushUpdatePullSweep(): void {
    StochasticRelaxation.activeState().unShotFlux.clear();
    StochasticRelaxation.activeState().unShotYmp = 0.0;
    StochasticRelaxation.activeState().totalFlux.clear();
    StochasticRelaxation.activeState().totalYmp = 0.0;
    StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.clear();

    StochasticJacobi.stochasticJacobiPullRdEd(ElementHierarchyState.activeState().topCluster as StochasticRadiosityElement);

    StochasticJacobi.stochasticJacobiPushUpdatePull(ElementHierarchyState.activeState().topCluster as Element);
  }

  /**
Generic routine for Stochastic Jacobi iterations.
*/
  public static doStochasticJacobiIteration(
    sceneWorldVoxelGrid: VoxelGrid,
    numberOfRays: number,
    getRadianceCallBack: ((elem: StochasticRadiosityElement) => ColorRgb[] | null) | null,
    getImportanceCallBack: ((elem: StochasticRadiosityElement) => number) | null,
    updateCallBack: ((elem: StochasticRadiosityElement, w: number) => void) | null,
    scenePatches: ArrayList<Patch>,
    renderOptions: RenderOptions
  ): void {
    StochasticJacobi.stochasticJacobiInitGlobals(
      numberOfRays | 0,
      getRadianceCallBack,
      getImportanceCallBack,
      updateCallBack
    );
    StochasticJacobi.stochasticJacobiPrintMessage(numberOfRays);
    if (!StochasticJacobi.stochasticJacobiSetup(scenePatches)) {
      return;
    }
    StochasticJacobi.stochasticJacobiShootRays(sceneWorldVoxelGrid, scenePatches, renderOptions);
    StochasticJacobi.stochasticJacobiPushUpdatePullSweep();
  }
}
