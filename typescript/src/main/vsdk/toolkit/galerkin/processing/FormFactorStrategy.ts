import { ColorRgb } from "../../common/ColorRgb";
import { Matrix2x2 } from "../../common/linealAlgebra/Matrix2x2";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Ray } from "../../common/linealAlgebra/Ray";
import { Vector2D } from "../../common/linealAlgebra/Vector2D";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinClusteringStrategy } from "../GalerkinClusteringStrategy";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinState } from "../GalerkinState";
import { Interaction } from "../Interaction";
import { ShadowCache } from "../ShadowCache";
import { RayHitFlag } from "../../skin/RayHitFlag";
import { CubatureRule } from "../../numericalAnalysis/CubatureRule";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { BoundingBox } from "../../skin/BoundingBox";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../skin/Patch";
import { RayHit } from "../../skin/RayHit";
import { FormFactorClusteredStrategy } from "./FormFactorClusteredStrategy";

export class FormFactorStrategy {
  private static formFactorLastReceived: GalerkinElement | null;
  private static formFactorLastSource: GalerkinElement | null;

  private static shadowTestDiscretization(
    ray: Ray,
    geometrySceneList: Geometry[] | null,
    voxelGrid: VoxelGrid | null,
    shadowCache: ShadowCache,
    minimumDistance: number,
    hitStore: RayHit,
    isSceneGeometry: boolean,
    isClusteredGeometry: boolean,
  ): RayHit | null {
    Statistics.instance().shadow.numberOfShadowRays++;
    const dist = [minimumDistance];
    let hit = shadowCache.cacheHit(ray, dist, hitStore);
    if (hit !== null) {
      Statistics.instance().shadow.numberOfShadowCacheHits++;
      return hit;
    }

    if (!isClusteredGeometry && !isSceneGeometry) {
      hit = Geometry.listDiscretizationIntersect(
        geometrySceneList,
        ray,
        Numeric.EPSILON_FLOAT * dist[0],
        dist,
        RayHitFlag.FRONT | RayHitFlag.ANY,
        hitStore,
      );
    }
    else if (voxelGrid !== null) {
      hit = voxelGrid.gridIntersect(
        ray,
        Numeric.EPSILON_FLOAT * dist[0],
        dist,
        RayHitFlag.FRONT | RayHitFlag.ANY,
        hitStore,
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
    x: Vector3D[],
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
          x[k] = new Vector3D();
        }
        x[k].set(
          minX + (cr[0] as CubatureRule).u[k] * dx,
          minY + (cr[0] as CubatureRule).v[k] * dy,
          minZ + (cr[0] as CubatureRule).t[k] * dz,
        );
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

    const topTransform = new Matrix2x2();
    const hasTopTransform = element.transformToParent !== null && element.topTransform(topTransform) !== null;

    for (let k = 0; cr[0] !== null && k < (cr[0] as CubatureRule).numberOfNodes && k < x.length; k++) {
      if (x[k] === null) {
        x[k] = new Vector3D();
      }
      const node = new Vector2D((cr[0] as CubatureRule).u[k], (cr[0] as CubatureRule).v[k]);
      if (hasTopTransform) {
        topTransform.transformPoint2D(node, node);
      }
      (element.patch as Patch).uniformPoint(node.x, node.y, x[k]);
    }
  }

  private static evaluatePointsPairKernel(
    shadowCache: ShadowCache,
    sceneWorldVoxelGrid: VoxelGrid | null,
    x: Vector3D,
    y: Vector3D,
    receiverElement: GalerkinElement,
    sourceElement: GalerkinElement,
    shadowGeometryList: Geometry[] | null,
    isSceneGeometry: boolean,
    isClusteredGeometry: boolean,
    galerkinState: GalerkinState,
  ): number {
    const ray = new Ray();
    ray.position.copy(y);
    ray.direction.subtraction(x, y);
    const distance = ray.direction.norm();
    if (distance < Numeric.EPSILON) {
      return 0.0;
    }
    ray.direction.inverseScaledCopy(distance, ray.direction, Numeric.EPSILON_FLOAT);

    let cosThetaY: number;
    if (sourceElement.isCluster()) {
      cosThetaY = 0.25;
    }
    else {
      cosThetaY = ray.direction.dotProduct((sourceElement.patch as Patch).normal);
      if (cosThetaY <= 0.0) {
        return 0.0;
      }
    }

    let cosThetaX: number;
    if (receiverElement.isCluster()) {
      cosThetaX = 0.25;
    }
    else {
      cosThetaX = -ray.direction.dotProduct((receiverElement.patch as Patch).normal);
      if (cosThetaX <= 0.0) {
        return 0.0;
      }
    }

    const formFactorKernelTerm = cosThetaX * cosThetaY / (globalThis.Math.PI * distance * distance);
    const shortenedDistance = distance * (1.0 - Numeric.EPSILON);

    let visibilityFactor: number;
    if (shadowGeometryList === null || shadowGeometryList.length === 0) {
      visibilityFactor = 1.0;
    }
    else if (galerkinState.multiResolutionVisibility === 0) {
      const hitStore = new RayHit();
      const h = FormFactorStrategy.shadowTestDiscretization(
        ray,
        shadowGeometryList,
        sceneWorldVoxelGrid,
        shadowCache,
        shortenedDistance,
        hitStore,
        isSceneGeometry,
        isClusteredGeometry,
      );
      visibilityFactor = (h === null) ? 1.0 : 0.0;
    }
    else if (shadowCache.cacheHit(ray, [shortenedDistance], new RayHit()) !== null) {
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
      );
    }

    return formFactorKernelTerm * visibilityFactor;
  }

  public static computeAreaToAreaFormFactorVisibility(
    sceneWorldVoxelGrid: VoxelGrid | null,
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
        link.deltaK = [1.0];
        link.numberOfReceiverCubaturePositions = 1;
        link.visibility = 128;
        return;
      }
    }
    else if (receiverElement === sourceElement) {
      link.deltaK = [0.0];
      link.numberOfReceiverCubaturePositions = 1;
      link.visibility = 0;
      return;
    }

    const receiverCubatureRuleRef: Array<CubatureRule | null> = [null];
    const sourceCubatureRuleRef: Array<CubatureRule | null> = [null];
    const x = new Array<Vector3D>(CubatureRule.MAXIMUM_NODES);
    const y = new Array<Vector3D>(CubatureRule.MAXIMUM_NODES);
    for (let i = 0; i < CubatureRule.MAXIMUM_NODES; i++) {
      x[i] = new Vector3D();
      y[i] = new Vector3D();
    }

    FormFactorStrategy.determineNodes(receiverElement, GalerkinRole.RECEIVER, galerkinState, receiverCubatureRuleRef, x);
    FormFactorStrategy.determineNodes(sourceElement, GalerkinRole.SOURCE, galerkinState, sourceCubatureRuleRef, y);

    const receiverCubatureRule = receiverCubatureRuleRef[0];
    const sourceCubatureRule = sourceCubatureRuleRef[0];
    if (receiverCubatureRule === null || sourceCubatureRule === null) {
      link.deltaK = [0.0];
      link.numberOfReceiverCubaturePositions = 1;
      link.visibility = 0;
      return;
    }

    const shadowCache = new ShadowCache();
    Patch.dontIntersect4(
      receiverElement.isCluster() ? null : receiverElement.patch,
      (receiverElement.isCluster() || receiverElement.patch === null) ? null : receiverElement.patch.twin,
      sourceElement.isCluster() ? null : sourceElement.patch,
      (sourceElement.isCluster() || sourceElement.patch === null) ? null : sourceElement.patch.twin,
    );
    Geometry.dontIntersect(
      receiverElement.isCluster() ? receiverElement.geometry : null,
      sourceElement.isCluster() ? sourceElement.geometry : null,
    );

    let maximumKernelValue = 0.0;
    const Gxy = new Array<number[]>(CubatureRule.MAXIMUM_NODES);
    for (let i = 0; i < CubatureRule.MAXIMUM_NODES; i++) {
      Gxy[i] = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
    }

    let visibilityCount = 0;
    for (let r = 0; r < receiverCubatureRule.numberOfNodes; r++) {
      for (let s = 0; s < sourceCubatureRule.numberOfNodes; s++) {
        Gxy[r][s] = FormFactorStrategy.evaluatePointsPairKernel(
          shadowCache,
          sceneWorldVoxelGrid,
          x[r],
          y[s],
          receiverElement,
          sourceElement,
          geometryShadowList,
          isSceneGeometry,
          isClusteredGeometry,
          galerkinState,
        );
        if (Gxy[r][s] > maximumKernelValue) {
          maximumKernelValue = Gxy[r][s];
        }
        if (globalThis.Math.abs(Gxy[r][s]) > Numeric.EPSILON) {
          visibilityCount++;
        }
      }
    }

    Patch.dontIntersect0();
    Geometry.dontIntersect(null, null);

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
      link.K[0] = 0.0;
      link.deltaK = [0.0];
      link.numberOfReceiverCubaturePositions = 1;
    }

    link.visibility = globalThis.Math.trunc((255.0 * visibilityCount)
      / (receiverCubatureRule.numberOfNodes * sourceCubatureRule.numberOfNodes));
    if (galerkinState.exactVisibility !== 0 && geometryShadowList !== null && link.visibility === 255) {
      link.visibility = 254;
    }

    if (galerkinState.clusteringStrategy === GalerkinClusteringStrategy.ISOTROPIC
      && (receiverElement.isCluster() || sourceElement.isCluster())) {
      link.deltaK = [maximumKernelValue * sourceElement.area];
      link.numberOfReceiverCubaturePositions = 1;
    }
    FormFactorStrategy.formFactorLastReceived = receiverElement;
    FormFactorStrategy.formFactorLastSource = sourceElement;
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
      ? (sourceElement.unShotRadiance as ColorRgb[])
      : (sourceElement.radiance as ColorRgb[]);

    const deltaRadiance = new Array<ColorRgb>(CubatureRule.MAXIMUM_NODES);
    for (let i = 0; i < deltaRadiance.length; i++) {
      deltaRadiance[i] = new ColorRgb();
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
      gMin[0],
      gMax[0],
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
    sourceRadiance: ColorRgb[],
    deltaRadiance: ColorRgb[],
    link: Interaction,
  ): void {
    link.deltaK = new Array<number>(1);
    if (sourceRadiance[0].isBlack()) {
      const gAverage = link.K[0] / receiverElement.area;
      link.deltaK[0] = gMax - gAverage;
      if (gAverage - gMin > link.deltaK[0]) {
        link.deltaK[0] = gAverage - gMin;
      }
    }
    else {
      link.deltaK[0] = 0.0;
      for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
        deltaRadiance[k].divide(deltaRadiance[k], sourceRadiance[0]);
        const delta = globalThis.Math.abs(deltaRadiance[k].maximumComponent());
        if (delta > link.deltaK[0]) {
          link.deltaK[0] = delta;
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
    sourceRadiance: ColorRgb[],
    gMin: number[],
    gMax: number[],
    deltaRadiance: ColorRgb[],
    twoPatchesInteraction: Interaction,
  ): void {
    const receiverPhi = new Array<number[]>(GalerkinBasis.MAX_BASIS_SIZE);
    for (let i = 0; i < GalerkinBasis.MAX_BASIS_SIZE; i++) {
      receiverPhi[i] = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
    }

    for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
      if (receiverElement.isCluster()) {
        receiverPhi[0][k] = 1.0;
      }
      else {
        for (let alpha = 0; alpha < twoPatchesInteraction.numberOfBasisFunctionsOnReceiver && receiverBasis !== null; alpha++) {
          receiverPhi[alpha][k] = receiverBasis.function[alpha](receiverCubatureRule.u[k], receiverCubatureRule.v[k]);
        }
      }
    }

    const sourcePhi = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
    for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
      deltaRadiance[k].clear();
    }

    for (let beta = 0; beta < twoPatchesInteraction.numberOfBasisFunctionsOnSource; beta++) {
      if (sourceElement.isCluster()) {
        for (let l = 0; l < sourceCubatureRule.numberOfNodes; l++) {
          sourcePhi[l] = 1.0;
        }
      }
      else {
        for (let l = 0; l < sourceCubatureRule.numberOfNodes && sourceBasis !== null; l++) {
          sourcePhi[l] = sourceBasis.function[beta](sourceCubatureRule.u[l], sourceCubatureRule.v[l]);
        }
      }

      const gBeta = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);
      const deltaBeta = new Array<number>(CubatureRule.MAXIMUM_NODES).fill(0.0);

      for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
        gBeta[k] = 0.0;
        for (let l = 0; l < sourceCubatureRule.numberOfNodes; l++) {
          gBeta[k] += sourceCubatureRule.w[l] * Gxy[k][l] * sourcePhi[l];
        }
        gBeta[k] *= sourceElement.area;
        deltaBeta[k] = -gBeta[k];
      }

      for (let alpha = 0; alpha < twoPatchesInteraction.numberOfBasisFunctionsOnReceiver; alpha++) {
        let gAlphaBeta = 0.0;
        for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
          gAlphaBeta += receiverCubatureRule.w[k] * receiverPhi[alpha][k] * gBeta[k];
        }
        twoPatchesInteraction.K[alpha * twoPatchesInteraction.numberOfBasisFunctionsOnSource + beta] =
          receiverElement.area * gAlphaBeta;
        for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
          deltaBeta[k] += gAlphaBeta * receiverPhi[alpha][k];
        }
      }

      for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
        deltaRadiance[k].addScaled(deltaRadiance[k], deltaBeta[k], sourceRadiance[beta]);
      }

      if (beta === 0) {
        for (let k = 0; k < receiverCubatureRule.numberOfNodes; k++) {
          if (gBeta[k] < gMin[0]) {
            gMin[0] = gBeta[k];
          }
          if (gBeta[k] > gMax[0]) {
            gMax[0] = gBeta[k];
          }
        }
      }
    }
  }
}
