import { ColorRgb } from "../../common/color/ColorRgb";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinClusteringStrategy } from "../GalerkinClusteringStrategy";
import { GalerkinErrorNorm } from "../GalerkinErrorNorm";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinShaftCullMode } from "../GalerkinShaftCullMode";
import { GalerkinState } from "../GalerkinState";
import { Interaction } from "../Interaction";
import { Shaft } from "../Shaft";
import { Scene } from "../../scene/Scene";
import { Polygon } from "../../scene/Polygon";
import { ElementFlags } from "../../environment/geometry/elements/ElementFlags";
import { BoundingBox } from "../../skin/AxisAlignedBoundingBox";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ClusterTraversalStrategy } from "./ClusterTraversalStrategy";
import { FormFactorStrategy } from "./FormFactorStrategy";
import { InteractionEvaluationCode } from "./InteractionEvaluationCode";

export class HierarchicalRefinementStrategy {
  private static readonly hierarchicalKPool: number[][] = [];

  private static borrowKBuffer(): number[] {
    const v = HierarchicalRefinementStrategy.hierarchicalKPool.pop();
    if (v !== undefined) {
      return v;
    }
    return new Array<number>(GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE).fill(0.0);
  }

  private static returnKBuffer(v: number[] | null): void {
    if (v !== null && v.length === GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE) {
      HierarchicalRefinementStrategy.hierarchicalKPool.push(v);
    }
  }

  private static refineTreeDepth = 0;

  private static hierarchicRefinementCull(
    scene: Scene,
    candidatesList: Array<Geometry[] | null>,
    interaction: Interaction,
    isClusteredGeometry: boolean,
    galerkinState: GalerkinState,
  ): void {
    if (candidatesList[0] === null) {
      return;
    }

    if (galerkinState.shaftCullMode === GalerkinShaftCullMode.DO_SHAFT_CULLING_FOR_REFINEMENT
      || galerkinState.shaftCullMode === GalerkinShaftCullMode.ALWAYS_DO_SHAFT_CULLING) {
      const shaft = new Shaft();
      const rcvPolygon = new Polygon();
      const srcPolygon = new Polygon();
      const srcBounds = new BoundingBox();
      const rcvBounds = new BoundingBox();

      if (galerkinState.exactVisibility !== 0
        && !interaction.receiverElement.isCluster()
        && !interaction.sourceElement.isCluster()) {
        interaction.receiverElement.initPolygon(rcvPolygon);
        interaction.sourceElement.initPolygon(srcPolygon);
        shaft.constructFromPolygonToPolygon(rcvPolygon, srcPolygon);
      }
      else {
        shaft.constructFromBoundingBoxes(
          interaction.receiverElement.bounds(rcvBounds) as BoundingBox,
          interaction.sourceElement.bounds(srcBounds) as BoundingBox,
        );
      }

      if (interaction.receiverElement.isCluster()) {
        shaft.setShaftDontOpen(interaction.receiverElement.geometry);
      }
      else {
        shaft.setShaftOmit(interaction.receiverElement.patch);
      }

      if (interaction.sourceElement.isCluster()) {
        shaft.setShaftDontOpen(interaction.sourceElement.geometry);
      }
      else {
        shaft.setShaftOmit(interaction.sourceElement.patch);
      }

      const arr: Geometry[] = [];
      if (isClusteredGeometry) {
        shaft.cullGeometry(scene.clusteredRootGeometry, arr, galerkinState.shaftCullStrategy);
      }
      else {
        shaft.doCulling(candidatesList[0] ?? null, arr, galerkinState.shaftCullStrategy);
      }
      candidatesList[0] = arr;
    }
  }

  private static hierarchicRefinementUnCull(
    candidatesList: Array<Geometry[] | null>,
    galerkinState: GalerkinState,
  ): void {
    if (galerkinState.shaftCullMode === GalerkinShaftCullMode.DO_SHAFT_CULLING_FOR_REFINEMENT
      || galerkinState.shaftCullMode === GalerkinShaftCullMode.ALWAYS_DO_SHAFT_CULLING) {
      Shaft.freeCandidateList(candidatesList[0] ?? null);
    }
  }

  private static hierarchicRefinementColorToError(radiance: ColorRgb): number {
    return radiance !== null ? radiance.maximumComponent() : 0.0;
  }

  private static hierarchicRefinementLinkErrorThreshold(
    interaction: Interaction,
    receiverArea: number,
    galerkinState: GalerkinState,
  ): number {
    let threshold: number;
    switch (galerkinState.errorNorm) {
      case GalerkinErrorNorm.RADIANCE_ERROR:
        threshold = HierarchicalRefinementStrategy.hierarchicRefinementColorToError(
          Statistics.instance().radiance.maxSelfEmittedRadiance,
        ) * galerkinState.relLinkErrorThreshold;
        break;
      case GalerkinErrorNorm.POWER_ERROR:
        threshold = HierarchicalRefinementStrategy.hierarchicRefinementColorToError(
          Statistics.instance().radiance.maxSelfEmittedPower,
        ) * galerkinState.relLinkErrorThreshold / (globalThis.Math.PI * receiverArea);
        break;
      default:
        threshold = galerkinState.relLinkErrorThreshold;
        break;
    }

    if (galerkinState.importanceDriven !== 0
      && (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI
        || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL)) {
      const maxDirect = Statistics.instance().potential.maxDirectPotential;
      if (maxDirect > Numeric.EPSILON) {
        threshold /= 2.0 * interaction.receiverElement.potential / maxDirect;
      }
    }
    return threshold;
  }

  private static hierarchicRefinementApproximationError(
    interaction: Interaction,
    srcRho: ColorRgb,
    rcvRho: ColorRgb,
    galerkinState: GalerkinState,
  ): number {
    const error = new ColorRgb();
    let srcRad: ColorRgb;

    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
      || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI) {
      if (interaction.sourceElement.isCluster()
        && interaction.sourceElement !== interaction.receiverElement) {
        srcRad = ClusterTraversalStrategy.maxRadiance(interaction.sourceElement, galerkinState);
      }
      else {
        srcRad = (interaction.sourceElement.radiance as ColorRgb[])[0]!;
      }
    }
    else {
      if (interaction.sourceElement.isCluster()
        && interaction.sourceElement !== interaction.receiverElement) {
        srcRad = ClusterTraversalStrategy.sourceClusterRadiance(interaction, galerkinState);
      }
      else {
        srcRad = (interaction.sourceElement.unShotRadiance as ColorRgb[])[0]!;
      }
    }

    error.scalarProductScaled(rcvRho, interaction.deltaK[0]!, srcRad);
    error.abs();
    return HierarchicalRefinementStrategy.hierarchicRefinementColorToError(error);
  }

  private static sourceClusterRadianceVariationError(
    interaction: Interaction,
    rcvRho: ColorRgb,
    receiverArea: number,
    galerkinState: GalerkinState,
  ): number {
    const K = interaction.K[0]!;
    if (K === 0.0 || rcvRho.isBlack() || (interaction.sourceElement.radiance as ColorRgb[])[0]!.isBlack()) {
      return 0.0;
    }

    const rcVertices: Vector3D[] = [
      new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(),
      new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(),
    ];
    const numberOfReceiverVertices = interaction.receiverElement.vertices(rcVertices);

    const minimumSourceRadiance = new ColorRgb();
    const maximumSourceRadiance = new ColorRgb();
    const error = new ColorRgb();
    minimumSourceRadiance.setMonochrome(Numeric.HUGE_FLOAT_VALUE);
    maximumSourceRadiance.setMonochrome(-Numeric.HUGE_FLOAT_VALUE);

    for (let i = 0; i < numberOfReceiverVertices; i++) {
      const radiance = ClusterTraversalStrategy.clusterRadianceToSamplePoint(
        interaction.sourceElement,
        rcVertices[i]!,
        galerkinState,
      );
      minimumSourceRadiance.minimum(minimumSourceRadiance, radiance);
      maximumSourceRadiance.maximum(maximumSourceRadiance, radiance);
    }
    error.subtract(maximumSourceRadiance, minimumSourceRadiance);
    error.scalarProductScaled(rcvRho, K / receiverArea, error);
    error.abs();
    return HierarchicalRefinementStrategy.hierarchicRefinementColorToError(error);
  }

  private static hierarchicRefinementEvaluateInteraction(
    interaction: Interaction,
    galerkinState: GalerkinState,
  ): InteractionEvaluationCode {
    if (!galerkinState.hierarchical) {
      return InteractionEvaluationCode.ACCURATE_ENOUGH;
    }

    let receiverArea: number;
    const rcvRho = new ColorRgb();
    if (interaction.receiverElement.isCluster()) {
      rcvRho.setMonochrome(1.0);
      receiverArea = ClusterTraversalStrategy.receiverArea(interaction, galerkinState);
    }
    else {
      rcvRho.set(
        interaction.receiverElement.patch!.radianceData!.Rd.r,
        interaction.receiverElement.patch!.radianceData!.Rd.g,
        interaction.receiverElement.patch!.radianceData!.Rd.b,
      );
      receiverArea = interaction.receiverElement.area;
    }

    const srcRho = new ColorRgb();
    if (interaction.sourceElement.isCluster()) {
      srcRho.setMonochrome(1.0);
    }
    else {
      srcRho.set(
        interaction.sourceElement.patch!.radianceData!.Rd.r,
        interaction.sourceElement.patch!.radianceData!.Rd.g,
        interaction.sourceElement.patch!.radianceData!.Rd.b,
      );
    }

    const threshold = HierarchicalRefinementStrategy.hierarchicRefinementLinkErrorThreshold(
      interaction,
      receiverArea,
      galerkinState,
    );
    let error = HierarchicalRefinementStrategy.hierarchicRefinementApproximationError(
      interaction,
      srcRho,
      rcvRho,
      galerkinState,
    );

    if (interaction.sourceElement.isCluster()
      && error < threshold
      && galerkinState.clusteringStrategy !== GalerkinClusteringStrategy.ISOTROPIC) {
      error += HierarchicalRefinementStrategy.sourceClusterRadianceVariationError(
        interaction,
        rcvRho,
        receiverArea,
        galerkinState,
      );
    }

    const minimumArea = Statistics.instance().radiance.totalArea * galerkinState.relMinElemArea;

    if (error <= threshold) {
      return InteractionEvaluationCode.ACCURATE_ENOUGH;
    }

    if ((!(interaction.sourceElement.isCluster()
      && (interaction.sourceElement.flags & ElementFlags.IS_LIGHT_SOURCE_MASK) !== 0))
      && (receiverArea > interaction.sourceElement.area)) {
      if (receiverArea > minimumArea) {
        if (interaction.receiverElement.isCluster()) {
          return InteractionEvaluationCode.SUBDIVIDE_RECEIVER_CLUSTER;
        }
        return InteractionEvaluationCode.REGULAR_SUBDIVIDE_RECEIVER;
      }
    }
    else {
      if (interaction.sourceElement.isCluster()) {
        return InteractionEvaluationCode.SUBDIVIDE_SOURCE_CLUSTER;
      }
      else if (interaction.sourceElement.area > minimumArea) {
        return InteractionEvaluationCode.REGULAR_SUBDIVIDE_SOURCE;
      }
    }

    return InteractionEvaluationCode.ACCURATE_ENOUGH;
  }

  private static hierarchicRefinementComputeLightTransport(
    interaction: Interaction,
    galerkinState: GalerkinState,
  ): void {
    const a = globalThis.Math.min(interaction.numberOfBasisFunctionsOnReceiver, interaction.receiverElement.basisSize);
    const b = globalThis.Math.min(interaction.numberOfBasisFunctionsOnSource, interaction.sourceElement.basisSize);
    if (a > interaction.receiverElement.basisUsed) {
      interaction.receiverElement.basisUsed = a;
    }
    if (b > interaction.sourceElement.basisUsed) {
      interaction.sourceElement.basisUsed = b;
    }

    let srcRad: ColorRgb[];
    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
      srcRad = interaction.sourceElement.unShotRadiance as ColorRgb[];
    }
    else {
      srcRad = interaction.sourceElement.radiance as ColorRgb[];
    }

    if (interaction.sourceElement.isCluster() && interaction.sourceElement !== interaction.receiverElement) {
      const linkClusterRad = ClusterTraversalStrategy.sourceClusterRadiance(interaction, galerkinState);
      srcRad = [linkClusterRad];
    }

    if (interaction.receiverElement.isCluster() && interaction.sourceElement !== interaction.receiverElement) {
      ClusterTraversalStrategy.gatherRadiance(interaction, srcRad, galerkinState);
    }
    else {
      const rcvRad = interaction.receiverElement.receivedRadiance as ColorRgb[];
      if (interaction.numberOfBasisFunctionsOnReceiver === 1
        && interaction.numberOfBasisFunctionsOnSource === 1) {
        rcvRad[0]!.addScaled(rcvRad[0]!, interaction.K[0]!, srcRad[0]!);
      }
      else {
        for (let alpha = 0; alpha < a; alpha++) {
          for (let beta = 0; beta < b; beta++) {
            rcvRad[alpha]!.addScaled(
              rcvRad[alpha]!,
              interaction.K[alpha * interaction.numberOfBasisFunctionsOnSource + beta]!,
              srcRad[beta]!,
            );
          }
        }
      }
    }

    if (galerkinState.importanceDriven !== 0) {
      const K = interaction.K[0]!;
      const rcvRho = new ColorRgb();
      const srcRho = new ColorRgb();

      if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
        || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI) {
        if (interaction.receiverElement.isCluster()) {
          rcvRho.setMonochrome(1.0);
        }
        else {
          rcvRho.set(
            interaction.receiverElement.patch!.radianceData!.Rd.r,
            interaction.receiverElement.patch!.radianceData!.Rd.g,
            interaction.receiverElement.patch!.radianceData!.Rd.b,
          );
        }
        interaction.sourceElement.receivedPotential +=
          K * HierarchicalRefinementStrategy.hierarchicRefinementColorToError(rcvRho) * interaction.receiverElement.potential;
      }
      else if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
        if (interaction.sourceElement.isCluster()) {
          srcRho.setMonochrome(1.0);
        }
        else {
          srcRho.set(
            interaction.sourceElement.patch!.radianceData!.Rd.r,
            interaction.sourceElement.patch!.radianceData!.Rd.g,
            interaction.sourceElement.patch!.radianceData!.Rd.b,
          );
        }
        interaction.receiverElement.receivedPotential +=
          K * HierarchicalRefinementStrategy.hierarchicRefinementColorToError(srcRho) * interaction.sourceElement.unShotPotential;
      }
    }
  }

  private static hierarchicRefinementCreateSubdivisionLink(
    scene: Scene,
    candidatesList: Geometry[] | null,
    receiverElement: GalerkinElement,
    sourceElement: GalerkinElement,
    interaction: Interaction,
    galerkinState: GalerkinState,
  ): number {
    interaction.receiverElement = receiverElement;
    interaction.sourceElement = sourceElement;

    if (interaction.receiverElement.isCluster()) {
      interaction.numberOfBasisFunctionsOnReceiver = 1;
    }
    else {
      interaction.numberOfBasisFunctionsOnReceiver = receiverElement.basisSize;
    }

    if (interaction.sourceElement.isCluster()) {
      interaction.numberOfBasisFunctionsOnSource = 1;
    }
    else {
      interaction.numberOfBasisFunctionsOnSource = sourceElement.basisSize;
    }

    const isSceneGeometry = (candidatesList === scene.geometryList);
    const isClusteredGeometry = (candidatesList === scene.clusteredGeometryList);
    FormFactorStrategy.computeAreaToAreaFormFactorVisibility(
      scene.voxelGrid,
      candidatesList,
      isSceneGeometry,
      isClusteredGeometry,
      interaction,
      galerkinState,
    );
    return interaction.visibility !== 0 ? 1 : 0;
  }

  private static hierarchicRefinementStoreInteraction(
    interaction: Interaction,
    galerkinState: GalerkinState,
  ): void {
    const newInteraction = Interaction.interactionDuplicate(interaction);
    if (newInteraction === null) {
      return;
    }
    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
      if (interaction.sourceElement.interactions === null) {
        interaction.sourceElement.interactions = [];
      }
      interaction.sourceElement.interactions.push(newInteraction);
    }
    else {
      if (interaction.receiverElement.interactions === null) {
        interaction.receiverElement.interactions = [];
      }
      interaction.receiverElement.interactions.push(newInteraction);
    }
  }

  private static hierarchicRefinementRegularSubdivideSource(
    scene: Scene,
    candidatesList: Array<Geometry[] | null>,
    interaction: Interaction,
    isClusteredGeometry: boolean,
    galerkinState: GalerkinState,
  ): void {
    const backup = candidatesList[0];
    HierarchicalRefinementStrategy.hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    const sourceElement = interaction.sourceElement;
    const receiverElement = interaction.receiverElement;

    sourceElement.regularSubDivide();
    if (sourceElement.regularSubElements === null) {
      HierarchicalRefinementStrategy.hierarchicRefinementComputeLightTransport(interaction, galerkinState);
      return;
    }

    for (let i = 0; i < 4; i++) {
      if (!(sourceElement.regularSubElements[i] instanceof GalerkinElement)) {
        continue;
      }
      const child = sourceElement.regularSubElements[i] as GalerkinElement;
      const subInteraction = new Interaction();
      subInteraction.K = HierarchicalRefinementStrategy.borrowKBuffer();
      subInteraction.ownsK = false;
      if (HierarchicalRefinementStrategy.hierarchicRefinementCreateSubdivisionLink(
        scene,
        candidatesList[0] ?? null,
        receiverElement,
        child,
        subInteraction,
        galerkinState,
      ) !== 0
        && !HierarchicalRefinementStrategy.refineRecursive(scene, candidatesList, subInteraction, galerkinState)) {
        HierarchicalRefinementStrategy.hierarchicRefinementStoreInteraction(subInteraction, galerkinState);
      }
      const borrowed = subInteraction.K;
      subInteraction.K = [];
      HierarchicalRefinementStrategy.returnKBuffer(borrowed);
    }
    HierarchicalRefinementStrategy.hierarchicRefinementUnCull(candidatesList, galerkinState);
    candidatesList[0] = backup ?? null;
  }

  private static hierarchicRefinementRegularSubdivideReceiver(
    scene: Scene,
    candidatesList: Array<Geometry[] | null>,
    interaction: Interaction,
    isClusteredGeometry: boolean,
    galerkinState: GalerkinState,
  ): void {
    const backup = candidatesList[0];
    HierarchicalRefinementStrategy.hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    const sourceElement = interaction.sourceElement;
    const receiverElement = interaction.receiverElement;

    receiverElement.regularSubDivide();
    if (receiverElement.regularSubElements === null) {
      HierarchicalRefinementStrategy.hierarchicRefinementComputeLightTransport(interaction, galerkinState);
      return;
    }

    for (let i = 0; i < 4; i++) {
      if (!(receiverElement.regularSubElements[i] instanceof GalerkinElement)) {
        continue;
      }
      const child = receiverElement.regularSubElements[i] as GalerkinElement;
      const subInteraction = new Interaction();
      subInteraction.K = HierarchicalRefinementStrategy.borrowKBuffer();
      subInteraction.ownsK = false;
      if (HierarchicalRefinementStrategy.hierarchicRefinementCreateSubdivisionLink(
        scene,
        candidatesList[0] ?? null,
        child,
        sourceElement,
        subInteraction,
        galerkinState,
      ) !== 0
        && !HierarchicalRefinementStrategy.refineRecursive(scene, candidatesList, subInteraction, galerkinState)) {
        HierarchicalRefinementStrategy.hierarchicRefinementStoreInteraction(subInteraction, galerkinState);
      }
      const borrowed = subInteraction.K;
      subInteraction.K = [];
      HierarchicalRefinementStrategy.returnKBuffer(borrowed);
    }
    HierarchicalRefinementStrategy.hierarchicRefinementUnCull(candidatesList, galerkinState);
    candidatesList[0] = backup ?? null;
  }

  private static hierarchicRefinementSubdivideSourceCluster(
    scene: Scene,
    candidatesList: Array<Geometry[] | null>,
    interaction: Interaction,
    isClusteredGeometry: boolean,
    galerkinState: GalerkinState,
  ): void {
    const backup = candidatesList[0];
    HierarchicalRefinementStrategy.hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    const sourceElement = interaction.sourceElement;
    const receiverElement = interaction.receiverElement;

    for (let i = 0;
      sourceElement.irregularSubElements !== null && i < sourceElement.irregularSubElements.length;
      i++) {
      if (!(sourceElement.irregularSubElements[i] instanceof GalerkinElement)) {
        continue;
      }
      const childElement = sourceElement.irregularSubElements[i] as GalerkinElement;
      if (!childElement.isCluster()) {
        const patch = childElement.patch;
        if (patch !== null && (
          (receiverElement.isCluster() && receiverElement.geometry !== null
            && receiverElement.geometry.getBoundingBox().behindPlane(patch.normal, patch.planeConstant))
          || (!receiverElement.isCluster() && receiverElement.patch !== null
            && !receiverElement.patch.facing(patch))
        )) {
          continue;
        }
      }
      const subInteraction = new Interaction();
      subInteraction.K = HierarchicalRefinementStrategy.borrowKBuffer();
      subInteraction.ownsK = false;

      if (HierarchicalRefinementStrategy.hierarchicRefinementCreateSubdivisionLink(
        scene,
        candidatesList[0] ?? null,
        receiverElement,
        childElement,
        subInteraction,
        galerkinState,
      ) !== 0
        && !HierarchicalRefinementStrategy.refineRecursive(scene, candidatesList, subInteraction, galerkinState)) {
        HierarchicalRefinementStrategy.hierarchicRefinementStoreInteraction(subInteraction, galerkinState);
      }
      const borrowed = subInteraction.K;
      subInteraction.K = [];
      HierarchicalRefinementStrategy.returnKBuffer(borrowed);
    }
    HierarchicalRefinementStrategy.hierarchicRefinementUnCull(candidatesList, galerkinState);
    candidatesList[0] = backup ?? null;
  }

  private static hierarchicRefinementSubdivideReceiverCluster(
    scene: Scene,
    candidatesList: Array<Geometry[] | null>,
    interaction: Interaction,
    isClusteredGeometry: boolean,
    galerkinState: GalerkinState,
  ): void {
    const backup = candidatesList[0];
    HierarchicalRefinementStrategy.hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    const sourceElement = interaction.sourceElement;
    const receiverElement = interaction.receiverElement;

    for (let i = 0;
      receiverElement.irregularSubElements !== null && i < receiverElement.irregularSubElements.length;
      i++) {
      if (!(receiverElement.irregularSubElements[i] instanceof GalerkinElement)) {
        continue;
      }
      const child = receiverElement.irregularSubElements[i] as GalerkinElement;
      if (!child.isCluster()) {
        const patch = child.patch;
        if (patch !== null && (
          (sourceElement.isCluster() && sourceElement.geometry !== null
            && sourceElement.geometry.getBoundingBox().behindPlane(patch.normal, patch.planeConstant))
          || (!sourceElement.isCluster() && sourceElement.patch !== null
            && !sourceElement.patch.facing(patch))
        )) {
          continue;
        }
      }
      const subInteraction = new Interaction();
      subInteraction.K = HierarchicalRefinementStrategy.borrowKBuffer();
      subInteraction.ownsK = false;
      if (HierarchicalRefinementStrategy.hierarchicRefinementCreateSubdivisionLink(
        scene,
        candidatesList[0] ?? null,
        child,
        sourceElement,
        subInteraction,
        galerkinState,
      ) !== 0
        && !HierarchicalRefinementStrategy.refineRecursive(scene, candidatesList, subInteraction, galerkinState)) {
        HierarchicalRefinementStrategy.hierarchicRefinementStoreInteraction(subInteraction, galerkinState);
      }
      const borrowed = subInteraction.K;
      subInteraction.K = [];
      HierarchicalRefinementStrategy.returnKBuffer(borrowed);
    }
    HierarchicalRefinementStrategy.hierarchicRefinementUnCull(candidatesList, galerkinState);
    candidatesList[0] = backup ?? null;
  }

  private static refineRecursive(
    scene: Scene,
    candidatesList: Array<Geometry[] | null>,
    interaction: Interaction,
    galerkinState: GalerkinState,
  ): boolean {
    let refined: boolean;

    const isClusteredGeometry = (candidatesList[0] === scene.clusteredGeometryList);
    switch (HierarchicalRefinementStrategy.hierarchicRefinementEvaluateInteraction(interaction, galerkinState)) {
      case InteractionEvaluationCode.ACCURATE_ENOUGH:
        HierarchicalRefinementStrategy.hierarchicRefinementComputeLightTransport(interaction, galerkinState);
        refined = false;
        break;
      case InteractionEvaluationCode.REGULAR_SUBDIVIDE_SOURCE:
        HierarchicalRefinementStrategy.hierarchicRefinementRegularSubdivideSource(
          scene, candidatesList, interaction, isClusteredGeometry, galerkinState,
        );
        refined = true;
        break;
      case InteractionEvaluationCode.REGULAR_SUBDIVIDE_RECEIVER:
        HierarchicalRefinementStrategy.hierarchicRefinementRegularSubdivideReceiver(
          scene, candidatesList, interaction, isClusteredGeometry, galerkinState,
        );
        refined = true;
        break;
      case InteractionEvaluationCode.SUBDIVIDE_SOURCE_CLUSTER:
        HierarchicalRefinementStrategy.hierarchicRefinementSubdivideSourceCluster(
          scene, candidatesList, interaction, isClusteredGeometry, galerkinState,
        );
        refined = true;
        break;
      case InteractionEvaluationCode.SUBDIVIDE_RECEIVER_CLUSTER:
        HierarchicalRefinementStrategy.hierarchicRefinementSubdivideReceiverCluster(
          scene, candidatesList, interaction, isClusteredGeometry, galerkinState,
        );
        refined = true;
        break;
      default:
        refined = false;
        break;
    }

    return refined;
  }

  private static refineInteraction(
    scene: Scene,
    interaction: Interaction,
    galerkinState: GalerkinState,
  ): boolean {
    let candidateOccluderList: Geometry[] | null = scene.clusteredGeometryList;

    if (galerkinState.exactVisibility !== 0 && interaction.visibility === 255) {
      candidateOccluderList = null;
    }

    const arr: Array<Geometry[] | null> = [candidateOccluderList];
    return HierarchicalRefinementStrategy.refineRecursive(scene, arr, interaction, galerkinState);
  }

  private static removeRefinedInteractions(
    galerkinState: GalerkinState,
    interactionsToRemove: Interaction[],
  ): void {
    for (let i = 0; i < interactionsToRemove.length; i++) {
      const interaction = interactionsToRemove[i]!;
      if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
        if (interaction.sourceElement.interactions !== null) {
          const idx = interaction.sourceElement.interactions.indexOf(interaction);
          if (idx >= 0) {
            interaction.sourceElement.interactions.splice(idx, 1);
          }
        }
      }
      else {
        if (interaction.receiverElement.interactions !== null) {
          const idx = interaction.receiverElement.interactions.indexOf(interaction);
          if (idx >= 0) {
            interaction.receiverElement.interactions.splice(idx, 1);
          }
        }
      }
      Interaction.interactionDestroy(interaction);
    }
  }

  public static refineInteractions(
    scene: Scene,
    parentElement: GalerkinElement,
    galerkinState: GalerkinState,
  ): void {
    const activePath = new Set<number>();
    HierarchicalRefinementStrategy.refineInteractionsInternal(scene, parentElement, galerkinState, activePath);
  }

  private static refineInteractionsInternal(
    scene: Scene,
    parentElement: GalerkinElement,
    galerkinState: GalerkinState,
    activePath: Set<number>,
  ): void {
    if (parentElement === null) {
      return;
    }

    const treeDepth = HierarchicalRefinementStrategy.refineTreeDepth;

    if (activePath.has(parentElement.id)) {
      return;
    }

    activePath.add(parentElement.id);
    HierarchicalRefinementStrategy.refineTreeDepth = treeDepth + 1;
    try {
      for (let i = 0;
        parentElement.irregularSubElements !== null && i < parentElement.irregularSubElements.length;
        i++) {
        if (parentElement.irregularSubElements[i] instanceof GalerkinElement) {
          HierarchicalRefinementStrategy.refineInteractionsInternal(
            scene,
            parentElement.irregularSubElements[i] as GalerkinElement,
            galerkinState,
            activePath,
          );
        }
      }

      if (parentElement.regularSubElements !== null) {
        for (let i = 0; i < 4; i++) {
          if (parentElement.regularSubElements[i] instanceof GalerkinElement) {
            HierarchicalRefinementStrategy.refineInteractionsInternal(
              scene,
              parentElement.regularSubElements[i] as GalerkinElement,
              galerkinState,
              activePath,
            );
          }
        }
      }

      const interactionsToRemove: Interaction[] = [];
      for (let i = 0; parentElement.interactions !== null && i < parentElement.interactions.length; i++) {
        const o = parentElement.interactions[i];
        if (!(o instanceof Interaction)) {
          continue;
        }
        const interaction = o as Interaction;
        if (HierarchicalRefinementStrategy.refineInteraction(scene, interaction, galerkinState)) {
          interactionsToRemove.push(interaction);
        }
      }
      HierarchicalRefinementStrategy.removeRefinedInteractions(galerkinState, interactionsToRemove);
    }
    finally {
      activePath.delete(parentElement.id);
      HierarchicalRefinementStrategy.refineTreeDepth = treeDepth;
    }
  }
}
