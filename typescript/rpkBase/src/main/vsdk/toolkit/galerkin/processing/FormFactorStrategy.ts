import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector2Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector2Df";
import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";
import { Statistics } from "../../common/statistics/Statistics";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinClusteringStrategy } from "../GalerkinClusteringStrategy";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinState } from "../GalerkinState";
import { Interaction } from "../Interaction";
import { ShadowCache } from "../ShadowCache";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
import { CubatureRule } from "../../numericalAnalysis/CubatureRule";
import { SceneQuery } from "../../scene/SceneQuery";
import type { SceneQueryWorkspace } from "../../scene/SceneQuery";
import { BoundingBox } from "../../skin/AxisAlignedBoundingBox";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../environment/geometry/elements/Patch";
import { RayExclusionSet } from "../../environment/geometry/elements/RayExclusionSet";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { FormFactorClusteredStrategy } from "./FormFactorClusteredStrategy";
import type { FormFactorWorkspace } from "./FormFactorWorkspace";
import { VisibilityQueryRecorder } from "../../scene/VisibilityQueryRecorder";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class FormFactorStrategy {
  private static cachedShadowTest(
    shadowCache: ShadowCache,
    ray: Ray,
    shortenedDistance: number,
    exclusions: RayExclusionSet,
    workspace: FormFactorWorkspace,
  ): boolean {
    const distance = workspace.getKernelDistance();
    distance[0] = shortenedDistance;
    return shadowCache.cacheHit(ray, distance, workspace.getKernelHitStore(), exclusions) !== null;
  }

  private static shadowTestDiscretization(
    ray: Ray,
    geometrySceneList: Geometry[] | null,
    sceneQuery: SceneQuery | null,
    sceneQueryWorkspace: SceneQueryWorkspace | null,
    shadowCache: ShadowCache,
    minimumDistance: number,
    hitStore: RayHit,
    isSceneGeometry: boolean,
    isClusteredGeometry: boolean,
    exclusions: RayExclusionSet,
    workspace: FormFactorWorkspace,
  ): RayHit | null {
    const shadowStatistics = workspace.getShadowStatistics();
    shadowStatistics.numberOfShadowRays++;
    const dist = workspace.getShadowTestDistance();
    dist[0] = minimumDistance;
    let hit = shadowCache.cacheHit(ray, dist, hitStore, exclusions);
    if (hit !== null) {
      shadowStatistics.numberOfShadowCacheHits++;
      if (sceneQueryWorkspace !== null && sceneQueryWorkspace.getRecorder() !== null) {
        sceneQueryWorkspace.getRecorder()!.countCacheResolved();
      }
      return hit;
    }

    const queryMaximumDistance = dist[0]!;

    if (!isClusteredGeometry && !isSceneGeometry) {
      hit = Geometry.listDiscretizationIntersect(
        geometrySceneList,
        ray,
        Math.fround(Numeric.EPSILON_FLOAT * dist[0]!),
        dist,
        RayHitFlag.FRONT | RayHitFlag.ANY,
        hitStore,
        exclusions,
      );
    }
    else if (sceneQuery !== null) {
      hit = sceneQuery.intersect(
        ray,
        Math.fround(Numeric.EPSILON_FLOAT * dist[0]!),
        dist,
        RayHitFlag.FRONT | RayHitFlag.ANY,
        hitStore,
        exclusions,
        sceneQueryWorkspace!,
      );
    }
    if (sceneQueryWorkspace !== null && sceneQueryWorkspace.getRecorder() !== null) {
      sceneQueryWorkspace.getRecorder()!.recordQuery(
        (!isClusteredGeometry && !isSceneGeometry)
          ? VisibilityQueryRecorder.GALERKIN_RESTRICTED_LIST : VisibilityQueryRecorder.GALERKIN_FULL_SCENE,
        ray,
        Math.fround(Numeric.EPSILON_FLOAT * queryMaximumDistance),
        queryMaximumDistance,
        RayHitFlag.FRONT | RayHitFlag.ANY,
        exclusions,
        hit,
        dist[0]!,
      );
    }

    if (hit !== null && hit.getPatch() !== null) {
      shadowCache.addToShadowCache(hit.getPatch() as Patch);
    }
    return hit;
  }

  private static determineNodes(
    element: GalerkinElement,
    role: GalerkinRole,
    galerkinState: GalerkinState,
    cr: Array<CubatureRule | null>,
    x: Vector3Df[],
  ): void {
    if (element === null || cr === null || cr.length === 0 || x === null) {
      return;
    }

    if (element.isCluster()) {
      cr[0] = galerkinState.clusterRule;
      const bb = new BoundingBox();
      element.bounds(bb);
      const dx = bb.dx();
      const dy = bb.dy();
      const dz = bb.dz();
      const minX = bb.minX();
      const minY = bb.minY();
      const minZ = bb.minZ();
      for (let k = 0; cr[0] !== null && k < (cr[0] as CubatureRule).numberOfNodes && k < x.length; k++) {
        if (x[k] === null) {
          x[k] = new Vector3Df();
        }
        x[k]! = new Vector3Df(minX + (cr[0] as CubatureRule).u[k]! * dx, minY + (cr[0] as CubatureRule).v[k]! * dy, minZ + (cr[0] as CubatureRule).t[k]! * dz);
      }
      return;
    }

    switch ((element.patch as Patch).numberOfVertices) {
      case 3:
        cr[0] = (role === GalerkinRole.RECEIVER)
          ? galerkinState.receiverTriangleCubatureRule
          : galerkinState.sourceTriangleCubatureRule;
        break;
      case 4:
        cr[0] = (role === GalerkinRole.RECEIVER)
          ? galerkinState.receiverQuadCubatureRule
          : galerkinState.sourceQuadCubatureRule;
        break;
      default:
        cr[0] = null;
        return;
    }

    const topTransform = element.topTransform();

    for (let k = 0; cr[0] !== null && k < (cr[0] as CubatureRule).numberOfNodes && k < x.length; k++) {
      if (x[k] === null) {
        x[k] = new Vector3Df();
      }
      let node = new Vector2Df((cr[0] as CubatureRule).u[k]!, (cr[0] as CubatureRule).v[k]!);
      if (topTransform !== null) {
        node = topTransform.transformPoint(node);
      }
      x[k] = (element.patch as Patch).uniformPoint(node.x(), node.y()) ?? x[k]!;
    }
  }

  private static evaluatePointsPairKernel(
    shadowCache: ShadowCache,
    sceneQuery: SceneQuery | null,
    sceneQueryWorkspace: SceneQueryWorkspace | null,
    x: Vector3Df,
    y: Vector3Df,
    receiverElement: GalerkinElement,
    sourceElement: GalerkinElement,
    shadowGeometryList: Geometry[] | null,
    isSceneGeometry: boolean,
    isClusteredGeometry: boolean,
    galerkinState: GalerkinState,
    exclusions: RayExclusionSet,
    workspace: FormFactorWorkspace,
  ): number {
    const ray = workspace.getKernelRay();
    ray.position = y;
    ray.direction = x.subtract(y);
    const distance = ray.direction.length();
    if (distance < VSDK.EPSILON) {
      return 0.0;
    }
    ray.direction = ray.direction.multiplyByInverse(distance, Numeric.EPSILON_FLOAT);

    let cosThetaY: number;
    if (sourceElement.isCluster()) {
      cosThetaY = 0.25;
    }
    else {
      cosThetaY = ray.direction.dotProduct((sourceElement.patch as Patch).getNormal());
      if (cosThetaY <= 0.0) {
        return 0.0;
      }
    }

    let cosThetaX: number;
    if (receiverElement.isCluster()) {
      cosThetaX = 0.25;
    }
    else {
      cosThetaX = -ray.direction.dotProduct((receiverElement.patch as Patch).getNormal());
      if (cosThetaX <= 0.0) {
        return 0.0;
      }
    }

    const formFactorKernelTerm = cosThetaX * cosThetaY / (globalThis.Math.PI * distance * distance);
    const shortenedDistance = distance * (1.0 - VSDK.EPSILON);

    let visibilityFactor: number;
    if (shadowGeometryList === null || shadowGeometryList.length === 0) {
      visibilityFactor = 1.0;
    }
    else if (galerkinState.multiResolutionVisibility === 0) {
      const hitStore = workspace.getKernelHitStore();
      const h = FormFactorStrategy.shadowTestDiscretization(
        ray,
        shadowGeometryList,
        sceneQuery,
        sceneQueryWorkspace,
        shadowCache,
        shortenedDistance,
        hitStore,
        isSceneGeometry,
        isClusteredGeometry,
        exclusions,
        workspace,
      );
      visibilityFactor = (h === null) ? 1.0 : 0.0;
    }
    else if (FormFactorStrategy.cachedShadowTest(shadowCache, ray, shortenedDistance, exclusions, workspace)) {
      if (sceneQueryWorkspace !== null && sceneQueryWorkspace.getRecorder() !== null) {
        sceneQueryWorkspace.getRecorder()!.countCacheResolved();
      }
      visibilityFactor = 0.0;
    }
    else {
      const minimumFeatureSize = 2.0
        * globalThis.Math.sqrt(Statistics.instance().radiance.totalArea * galerkinState.relMinElemArea / globalThis.Math.PI);
      visibilityFactor = FormFactorClusteredStrategy.geomListMultiResolutionVisibility(
        shadowGeometryList,
        shadowCache,
        ray,
        shortenedDistance,
        sourceElement.blockerSize,
        minimumFeatureSize,
        exclusions,
      );
    }

    return formFactorKernelTerm * visibilityFactor;
  }

  public static computeAreaToAreaFormFactorVisibility(
    sceneQuery: SceneQuery | null,
    sceneQueryWorkspace: SceneQueryWorkspace | null,
    formFactorWorkspace: FormFactorWorkspace,
    geometryShadowList: Geometry[] | null,
    isSceneGeometry: boolean,
    isClusteredGeometry: boolean,
    link: Interaction,
    galerkinState: GalerkinState,
  ): void {
    const receiverElement = link.receiverElement;
    const sourceElement = link.sourceElement;
    if (receiverElement === null || sourceElement === null) {
      link.visibility = 0;
      return;
    }

    let m = link.numberOfBasisFunctionsOnReceiver * link.numberOfBasisFunctionsOnSource;
    if (m <= 0) {
      m = 1;
    }
    if (link.K == null || link.K.length < m) {
      link.K = new Array<number>(m).fill(0.0);
    }
    for (let i = 0; i < m; i++) {
      link.K[i] = 0.0;
    }

    if (receiverElement.isCluster() || sourceElement.isCluster()) {
      const sourceBoundingBox = new BoundingBox();
      const receiverBoundingBox = new BoundingBox();
      receiverElement.bounds(receiverBoundingBox);
      sourceElement.bounds(sourceBoundingBox);
      if (!receiverBoundingBox.disjointToOtherBoundingBox(sourceBoundingBox)) {
        link.deltaK = 1.0;
        link.numberOfReceiverCubaturePositions = 1;
        link.visibility = 128;
        return;
      }
    }
    else if (receiverElement === sourceElement) {
      link.deltaK = 0.0;
      link.numberOfReceiverCubaturePositions = 1;
      link.visibility = 0;
      return;
    }

    // If the receiver is another one than before, determine the cubature rule to be
    // used on it and the nodes (positions on the patch); same for the source element
    // The cubature rules, nodes and kernel values of the last pair are kept in
    // the form factor workspace
    const cachedX = formFactorWorkspace.getReceiverNodes();
    const cachedY = formFactorWorkspace.getSourceNodes();
    const cachedGxy = formFactorWorkspace.getKernel();
    const sameReceiver = receiverElement === formFactorWorkspace.getLastReceiver();
    const sameSource = sourceElement === formFactorWorkspace.getLastSource();
    if (!sameReceiver) {
      const receiverCubatureRuleRef: Array<CubatureRule | null> = [formFactorWorkspace.getReceiverCubatureRule()];
      FormFactorStrategy.determineNodes(receiverElement, GalerkinRole.RECEIVER, galerkinState, receiverCubatureRuleRef, cachedX);
      formFactorWorkspace.setReceiverCubatureRule(receiverCubatureRuleRef[0] ?? null);
    }
    if (!sameSource) {
      const sourceCubatureRuleRef: Array<CubatureRule | null> = [formFactorWorkspace.getSourceCubatureRule()];
      FormFactorStrategy.determineNodes(sourceElement, GalerkinRole.SOURCE, galerkinState, sourceCubatureRuleRef, cachedY);
      formFactorWorkspace.setSourceCubatureRule(sourceCubatureRuleRef[0] ?? null);
    }

    const receiverCubatureRule = formFactorWorkspace.getReceiverCubatureRule();
    const sourceCubatureRule = formFactorWorkspace.getSourceCubatureRule();
    if (receiverCubatureRule === null || sourceCubatureRule === null) {
      link.deltaK = 0.0;
      link.numberOfReceiverCubaturePositions = 1;
      link.visibility = 0;
      return;
    }

    // Evaluate the radiosity kernel between each pair of nodes on the source and the
    // receiver element only if at least the receiver or the source changed since last time
    if (!sameReceiver || !sameSource) {
      const shadowCache = new ShadowCache();
      // Exclude the patches in order to avoid immediate self-intersections
      const exclusions = new RayExclusionSet(
        receiverElement.isCluster() ? null : receiverElement.patch,
        (receiverElement.isCluster() || receiverElement.patch === null) ? null : receiverElement.patch.twin,
        sourceElement.isCluster() ? null : sourceElement.patch,
        (sourceElement.isCluster() || sourceElement.patch === null) ? null : sourceElement.patch.twin,
        receiverElement.isCluster() ? receiverElement.geometry : null,
        sourceElement.isCluster() ? sourceElement.geometry : null,
      );

      if (sceneQueryWorkspace !== null && sceneQueryWorkspace.getRecorder() !== null
        && geometryShadowList !== null && geometryShadowList.length > 0
        && !isClusteredGeometry && !isSceneGeometry) {
        sceneQueryWorkspace.getRecorder()!.recordGeometryList(geometryShadowList);
      }

      let cachedMaximumKernelValue = 0.0;
      let cachedVisibilityCount = 0;
      for (let r = 0; r < receiverCubatureRule.numberOfNodes; r++) {
        for (let s = 0; s < sourceCubatureRule.numberOfNodes; s++) {
          cachedGxy[r]![s] = FormFactorStrategy.evaluatePointsPairKernel(
            shadowCache,
            sceneQuery,
            sceneQueryWorkspace,
            cachedX[r]!,
            cachedY[s]!,
            receiverElement,
            sourceElement,
            geometryShadowList,
            isSceneGeometry,
            isClusteredGeometry,
            galerkinState,
            exclusions,
            formFactorWorkspace,
          );
          if (cachedGxy[r]![s]! > cachedMaximumKernelValue) {
            cachedMaximumKernelValue = cachedGxy[r]![s]!;
          }
          if (globalThis.Math.abs(cachedGxy[r]![s]!) > VSDK.EPSILON) {
            cachedVisibilityCount++;
          }
        }
      }

      formFactorWorkspace.setMaximumKernelValue(cachedMaximumKernelValue);
      formFactorWorkspace.setVisibilityCount(cachedVisibilityCount);
    }

    const maximumKernelValue = formFactorWorkspace.getMaximumKernelValue();
    const Gxy = cachedGxy;
    const visibilityCount = formFactorWorkspace.getVisibilityCount();

    if (visibilityCount !== 0) {
      if (link.numberOfBasisFunctionsOnReceiver === 1 && link.numberOfBasisFunctionsOnSource === 1) {
        FormFactorClusteredStrategy.doConstantAreaToAreaFormFactor(
          link,
          receiverCubatureRule,
          sourceCubatureRule,
          Gxy,
        );
      }
      else {
        FormFactorStrategy.doHigherOrderAreaToAreaFormFactor(
          link,
          receiverCubatureRule,
          sourceCubatureRule,
          Gxy,
          galerkinState,
        );
      }
    }
    else {
      link.K[0]! = 0.0;
      link.deltaK = 0.0;
      link.numberOfReceiverCubaturePositions = 1;
    }

    link.visibility = globalThis.Math.trunc((255.0 * visibilityCount)
      / (receiverCubatureRule.numberOfNodes * sourceCubatureRule.numberOfNodes));
    if (galerkinState.exactVisibility !== 0 && geometryShadowList !== null && link.visibility === 255) {
      link.visibility = 254;
    }

    if (galerkinState.clusteringStrategy === GalerkinClusteringStrategy.ISOTROPIC
      && (receiverElement.isCluster() || sourceElement.isCluster())) {
      link.deltaK = maximumKernelValue * sourceElement.area;
      link.numberOfReceiverCubaturePositions = 1;
    }
    formFactorWorkspace.setLastPair(receiverElement, sourceElement);
  }

  private static doHigherOrderAreaToAreaFormFactor(
    twoPatchesInteraction: Interaction,
    receiverCubatureRule: CubatureRule,
    sourceCubatureRule: CubatureRule,
    Gxy: number[][],
    galerkinState: GalerkinState,
  ): void {
    const receiverElement = twoPatchesInteraction.receiverElement;
    const sourceElement = twoPatchesInteraction.sourceElement;

    const receiverBasis = receiverElement.isCluster()
      ? null
      : GalerkinBasis.basisForVertexCount((receiverElement.patch as Patch).numberOfVertices);
    const sourceBasis = sourceElement.isCluster()
      ? null
      : GalerkinBasis.basisForVertexCount((sourceElement.patch as Patch).numberOfVertices);

    const sourceRadiance = (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL)
      ? (sourceElement.unShotRadiance as ColorRgbMutable[])
      : (sourceElement.radiance as ColorRgbMutable[]);

    const deltaRadiance = new Array<ColorRgbMutable>(CubatureRule.MAXIMUM_NODES);
    for (let i = 0; i < deltaRadiance.length; i++) {
      deltaRadiance[i] = new ColorRgbMutable();
    }
    const gMin = [Numeric.HUGE_DOUBLE_VALUE];
    const gMax = [-Numeric.HUGE_DOUBLE_VALUE];

    FormFactorStrategy.computeInteractionFormFactor(
      receiverCubatureRule,
      sourceCubatureRule,
      Gxy,
      sourceElement,
      receiverElement,
      sourceBasis,
      receiverBasis,
      sourceRadiance,
      gMin,
      gMax,
      deltaRadiance,
      twoPatchesInteraction,
    );

    FormFactorStrategy.computeInteractionError(
      receiverCubatureRule,
      receiverElement,
      gMin[0]!,
      gMax[0]!,
      sourceRadiance,
      deltaRadiance,
      twoPatchesInteraction,
    );
  }

  private static computeInteractionError(
    receiverCubatureRule: CubatureRule,
    receiverElement: GalerkinElement,
    gMin: number,
    gMax: number,
    sourceRadiance: ColorRgbMutable[],
    deltaRadiance: ColorRgbMutable[],
    link: Interaction,
  ): void {
    if (sourceRadiance[0]!.isBlack()) {
      const gAverage = link.K[0]! / receiverElement.area;
      link.deltaK = gMax - gAverage;
      if (gAverage - gMin > link.deltaK) {
        link.deltaK = gAverage - gMin;
      }
    }
    else {
      link.deltaK = 0.0;
      for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
        deltaRadiance[k]!.divide(deltaRadiance[k]!, sourceRadiance[0]!);
        const delta = globalThis.Math.abs(deltaRadiance[k]!.maximumComponent());
        if (delta > link.deltaK) {
          link.deltaK = delta;
        }
      }
    }
    link.numberOfReceiverCubaturePositions = 1;
  }

  private static computeInteractionFormFactor(
    receiverCubatureRule: CubatureRule,
    sourceCubatureRule: CubatureRule,
    Gxy: number[][],
    sourceElement: GalerkinElement,
    receiverElement: GalerkinElement,
    sourceBasis: GalerkinBasis | null,
    receiverBasis: GalerkinBasis | null,
    sourceRadiance: ColorRgbMutable[],
    gMin: number[],
    gMax: number[],
    deltaRadiance: ColorRgbMutable[],
    twoPatchesInteraction: Interaction,
  ): void {
    const receiverPhi = new Array<number[]>(GalerkinBasis.MAX_BASIS_SIZE);
    for (let i = 0; i < GalerkinBasis.MAX_BASIS_SIZE; i++) {
      receiverPhi[i] = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
    }

    for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
      if (receiverElement.isCluster()) {
        receiverPhi[0]![k] = 1.0;
      }
      else {
        for (let alpha = 0; alpha < twoPatchesInteraction.numberOfBasisFunctionsOnReceiver && receiverBasis !== null; alpha++) {
          receiverPhi[alpha]![k] = receiverBasis.function[alpha]!(receiverCubatureRule.u[k]!, receiverCubatureRule.v[k]!);
        }
      }
    }

    const sourcePhi = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
    for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
      deltaRadiance[k]!.clear();
    }

    for (let beta = 0; beta < twoPatchesInteraction.numberOfBasisFunctionsOnSource; beta++) {
      if (sourceElement.isCluster()) {
        for (let l = 0; l < sourceCubatureRule.numberOfNodes; l++) {
          sourcePhi[l] = 1.0;
        }
      }
      else {
        for (let l = 0; l < sourceCubatureRule.numberOfNodes && sourceBasis !== null; l++) {
          sourcePhi[l] = sourceBasis.function[beta]!(sourceCubatureRule.u[l]!, sourceCubatureRule.v[l]!);
        }
      }

      const gBeta = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
      const deltaBeta = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);

      for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
        gBeta[k] = 0.0;
        for (let l = 0; l < sourceCubatureRule.numberOfNodes; l++) {
          gBeta[k]! += sourceCubatureRule.w[l]! * Gxy[k]![l]! * sourcePhi[l]!;
        }
        gBeta[k]! *= sourceElement.area;
        deltaBeta[k] = -gBeta[k]!;
      }

      for (let alpha = 0; alpha < twoPatchesInteraction.numberOfBasisFunctionsOnReceiver; alpha++) {
        let gAlphaBeta = 0.0;
        for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
          gAlphaBeta += receiverCubatureRule.w[k]! * receiverPhi[alpha]![k]! * gBeta[k]!;
        }
        twoPatchesInteraction.K[alpha * twoPatchesInteraction.numberOfBasisFunctionsOnSource + beta] =
          receiverElement.area * gAlphaBeta;
        for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
          deltaBeta[k]! += gAlphaBeta * receiverPhi[alpha]![k]!;
        }
      }

      for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
        deltaRadiance[k]!.addScaled(deltaRadiance[k]!, deltaBeta[k]!, sourceRadiance[beta]!);
      }

      if (beta === 0) {
        for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
          if (gBeta[k]! < gMin[0]!) {
            gMin[0] = gBeta[k]!;
          }
          if (gBeta[k]! > gMax[0]!) {
            gMax[0] = gBeta[k]!;
          }
        }
      }
    }
  }
}
