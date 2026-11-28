import { ColorRgbMutable } from "../../common/color/ColorRgbMutable";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { ShadowStatistics } from "../../common/statistics/ShadowStatistics";
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
import { ConvexPolygon } from "../../scene/ConvexPolygon";
import { ElementFlags } from "../../environment/geometry/elements/ElementFlags";
import { BoundingBox } from "../../skin/AxisAlignedBoundingBox";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ClusterTransportPlan } from "./ClusterTransportPlan";
import { ClusterTraversalStrategy } from "./ClusterTraversalStrategy";
import { ElementSamples } from "./ElementSamples";
import { FormFactorStrategy } from "./FormFactorStrategy";
import { FormFactorWorkspace } from "./FormFactorWorkspace";
import { InteractionEvaluationCode } from "./InteractionEvaluationCode";
import { RefinementReplicas } from "./RefinementReplicas";
import { ScratchVisibilityWorkspace } from "./ScratchVisibilityWorkspace";
import {
  RefinementEvent,
  RefinementGroup,
  RefinementRecord,
  RefinementScheduler,
  RefinementTask,
  RefinementWorkspace,
} from "./RefinementWorkspace";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";
import { Double } from "vitral/dist/java/lang/Double";

export class HierarchicalRefinementStrategy {
  private static readonly hierarchicalKPool: number[][] = [];

  private static borrowKBuffer(pool: number[][]): number[] {
    const v = pool.pop();
    if (v !== undefined) {
      return v;
    }
    return new Array<number>(GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE).fill(0.0);
  }

  private static returnKBuffer(pool: number[][], v: number[] | null): void {
    if (v !== null && v.length === GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE) {
      pool.push(v);
    }
  }

  private static prepareRefinementIterationState(galerkinState: GalerkinState): void {
    const statistics = Statistics.instance();
    galerkinState.refinementMinimumArea =
      statistics.radiance.totalArea * galerkinState.relMinElemArea;
    galerkinState.refinementRadianceErrorThreshold =
      HierarchicalRefinementStrategy.hierarchicRefinementColorToError(statistics.radiance.maxSelfEmittedRadiance)
      * galerkinState.relLinkErrorThreshold;
    galerkinState.refinementPowerErrorThresholdNumerator =
      HierarchicalRefinementStrategy.hierarchicRefinementColorToError(statistics.radiance.maxSelfEmittedPower)
      * galerkinState.relLinkErrorThreshold;
    galerkinState.refinementMaxDirectPotential =
      statistics.potential.maxDirectPotential;
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
      const shaft = new Shaft(scene.getFaceBounds()!);
      const rcvPolygon = new ConvexPolygon();
      const srcPolygon = new ConvexPolygon();
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

  private static hierarchicRefinementColorToError(radiance: ColorRgbMutable): number {
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
        threshold = galerkinState.refinementRadianceErrorThreshold;
        break;
      case GalerkinErrorNorm.POWER_ERROR:
        threshold = galerkinState.refinementPowerErrorThresholdNumerator / (globalThis.Math.PI * receiverArea);
        break;
      default:
        threshold = galerkinState.relLinkErrorThreshold;
        break;
    }

    if (galerkinState.importanceDriven !== 0
      && (galerkinState.isJacobi()
        || galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL)) {
      const maxDirect = galerkinState.refinementMaxDirectPotential;
      if (maxDirect > VSDK.EPSILON) {
        threshold /= 2.0 * interaction.receiverElement.potential / maxDirect;
      }
    }
    return threshold;
  }

  private static hierarchicRefinementApproximationError(
    interaction: Interaction,
    srcRho: ColorRgbMutable,
    rcvRho: ColorRgbMutable,
    scratch: ScratchVisibilityWorkspace | null,
    galerkinState: GalerkinState,
  ): number {
    const error = new ColorRgbMutable();
    let srcRad: ColorRgbMutable;

    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
      || galerkinState.isJacobi()) {
      if (interaction.sourceElement.isCluster()
        && interaction.sourceElement !== interaction.receiverElement) {
        srcRad = ClusterTraversalStrategy.maxRadiance(interaction.sourceElement, galerkinState);
      }
      else {
        srcRad = (interaction.sourceElement.radiance as ColorRgbMutable[])[0]!;
      }
    }
    else {
      if (interaction.sourceElement.isCluster()
        && interaction.sourceElement !== interaction.receiverElement) {
        srcRad = ClusterTraversalStrategy.sourceClusterRadiance(interaction, scratch, galerkinState);
      }
      else {
        srcRad = (interaction.sourceElement.unShotRadiance as ColorRgbMutable[])[0]!;
      }
    }

    error.scalarProductScaled(rcvRho, interaction.deltaK, srcRad);
    error.abs();
    return HierarchicalRefinementStrategy.hierarchicRefinementColorToError(error);
  }

  private static sourceClusterRadianceVariationError(
    interaction: Interaction,
    rcvRho: ColorRgbMutable,
    receiverArea: number,
    scratch: ScratchVisibilityWorkspace | null,
    galerkinState: GalerkinState,
  ): number {
    const K = interaction.K[0]!;
    if (K === 0.0 || rcvRho.isBlack() || (interaction.sourceElement.radiance as ColorRgbMutable[])[0]!.isBlack()) {
      return 0.0;
    }

    const rcVertices: Vector3Dd[] = [
      new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(),
      new Vector3Dd(), new Vector3Dd(), new Vector3Dd(), new Vector3Dd(),
    ];
    const numberOfReceiverVertices = interaction.receiverElement.vertices(rcVertices);

    const minimumSourceRadiance = new ColorRgbMutable();
    const maximumSourceRadiance = new ColorRgbMutable();
    const error = new ColorRgbMutable();
    minimumSourceRadiance.setMonochrome(Double.MAX_VALUE);
    maximumSourceRadiance.setMonochrome(-Double.MAX_VALUE);

    for (let i = 0; i < numberOfReceiverVertices; i++) {
      const radiance = ClusterTraversalStrategy.clusterRadianceToSamplePoint(
        interaction.sourceElement,
        rcVertices[i]!,
        scratch,
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
    scratch: ScratchVisibilityWorkspace | null,
    galerkinState: GalerkinState,
  ): InteractionEvaluationCode {
    if (!galerkinState.hierarchical) {
      return InteractionEvaluationCode.ACCURATE_ENOUGH;
    }

    let receiverArea: number;
    const rcvRho = new ColorRgbMutable();
    if (interaction.receiverElement.isCluster()) {
      rcvRho.setMonochrome(1.0);
      receiverArea = ClusterTraversalStrategy.receiverArea(interaction, scratch, galerkinState);
    }
    else {
      rcvRho.set(
        galerkinState.topLevelElement(interaction.receiverElement.patch!).Rd.r,
        galerkinState.topLevelElement(interaction.receiverElement.patch!).Rd.g,
        galerkinState.topLevelElement(interaction.receiverElement.patch!).Rd.b,
      );
      receiverArea = interaction.receiverElement.area;
    }

    const srcRho = new ColorRgbMutable();
    if (interaction.sourceElement.isCluster()) {
      srcRho.setMonochrome(1.0);
    }
    else {
      srcRho.set(
        galerkinState.topLevelElement(interaction.sourceElement.patch!).Rd.r,
        galerkinState.topLevelElement(interaction.sourceElement.patch!).Rd.g,
        galerkinState.topLevelElement(interaction.sourceElement.patch!).Rd.b,
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
      scratch,
      galerkinState,
    );

    if (interaction.sourceElement.isCluster()
      && error < threshold
      && galerkinState.clusteringStrategy !== GalerkinClusteringStrategy.ISOTROPIC) {
      error += HierarchicalRefinementStrategy.sourceClusterRadianceVariationError(
        interaction,
        rcvRho,
        receiverArea,
        scratch,
        galerkinState,
      );
    }

    const minimumArea = galerkinState.refinementMinimumArea;

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
    scratch: ScratchVisibilityWorkspace | null,
    galerkinState: GalerkinState,
  ): void {
    HierarchicalRefinementStrategy.receiverLightTransport(interaction, scratch, null, null, galerkinState);
    HierarchicalRefinementStrategy.sourceLightTransport(interaction, galerkinState);
  }

  /**
  The part of light transport that writes the receiver: the number of
  effectively used radiance coefficients and the received radiance. It reads
  only the source radiance (frozen during gathering). What the plan, when given,
  holds is used instead of computing it (see planClusterTransport)
  */
  private static receiverLightTransport(
    interaction: Interaction,
    scratch: ScratchVisibilityWorkspace | null,
    plan: ClusterTransportPlan | null,
    samplePools: ElementSamples[][] | null,
    galerkinState: GalerkinState,
  ): void {
    const a = globalThis.Math.min(interaction.numberOfBasisFunctionsOnReceiver, interaction.receiverElement.basisSize);
    const b = globalThis.Math.min(interaction.numberOfBasisFunctionsOnSource, interaction.sourceElement.basisSize);
    if (a > interaction.receiverElement.basisUsed) {
      interaction.receiverElement.basisUsed = a;
    }

    let srcRad: ColorRgbMutable[];
    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
      srcRad = interaction.sourceElement.unShotRadiance as ColorRgbMutable[];
    }
    else {
      srcRad = interaction.sourceElement.radiance as ColorRgbMutable[];
    }

    if (interaction.sourceElement.isCluster() && interaction.sourceElement !== interaction.receiverElement) {
      let linkClusterRad: ColorRgbMutable;
      if (plan !== null && plan.hasSourceRadiance()) {
        linkClusterRad = plan.getSourceRadiance();
      }
      else {
        linkClusterRad = ClusterTraversalStrategy.sourceClusterRadiance(interaction, scratch, galerkinState);
      }
      srcRad = [linkClusterRad];
    }

    if (interaction.receiverElement.isCluster() && interaction.sourceElement !== interaction.receiverElement) {
      if (plan !== null && plan.hasReceiverSamples()) {
        ClusterTraversalStrategy.gatherRadianceFromSamples(
          interaction, srcRad, plan.getPixelArea(),
          samplePools![plan.getPool()]!, plan.getFirstSample(), plan.getNumberOfSamples());
      }
      else {
        ClusterTraversalStrategy.gatherRadiance(interaction, srcRad, scratch, galerkinState);
      }
    }
    else {
      const rcvRad = interaction.receiverElement.receivedRadiance as ColorRgbMutable[];
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

    if (galerkinState.importanceDriven !== 0
      && galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
      const K = interaction.K[0]!;
      const srcRho = new ColorRgbMutable();
      if (interaction.sourceElement.isCluster()) {
        srcRho.setMonochrome(1.0);
      }
      else {
        srcRho.set(
          galerkinState.topLevelElement(interaction.sourceElement.patch!).Rd.r,
          galerkinState.topLevelElement(interaction.sourceElement.patch!).Rd.g,
          galerkinState.topLevelElement(interaction.sourceElement.patch!).Rd.b,
        );
      }
      interaction.receiverElement.receivedPotential +=
        K * HierarchicalRefinementStrategy.hierarchicRefinementColorToError(srcRho) * interaction.sourceElement.unShotPotential;
    }
  }

  /**
  The part of light transport that writes the source: the number of effectively
  used radiance coefficients and, for importance-driven gathering, the received
  potential. It reads only the receiver potential (frozen during gathering)
  */
  private static sourceLightTransport(
    interaction: Interaction,
    galerkinState: GalerkinState,
  ): void {
    const b = globalThis.Math.min(interaction.numberOfBasisFunctionsOnSource, interaction.sourceElement.basisSize);
    if (b > interaction.sourceElement.basisUsed) {
      interaction.sourceElement.basisUsed = b;
    }

    if (galerkinState.importanceDriven !== 0) {
      if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
        || galerkinState.isJacobi()) {
        const K = interaction.K[0]!;
        const rcvRho = new ColorRgbMutable();
        if (interaction.receiverElement.isCluster()) {
          rcvRho.setMonochrome(1.0);
        }
        else {
          rcvRho.set(
            galerkinState.topLevelElement(interaction.receiverElement.patch!).Rd.r,
            galerkinState.topLevelElement(interaction.receiverElement.patch!).Rd.g,
            galerkinState.topLevelElement(interaction.receiverElement.patch!).Rd.b,
          );
        }
        interaction.sourceElement.receivedPotential +=
          K * HierarchicalRefinementStrategy.hierarchicRefinementColorToError(rcvRho) * interaction.receiverElement.potential;
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
    workspace: RefinementWorkspace,
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
      scene.getSceneQuery(),
      workspace.getSceneQueryWorkspace(),
      workspace.getFormFactorWorkspace(),
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
    workspace: RefinementWorkspace,
  ): void {
    const record = workspace.getRecord();
    if (record !== null) {
      // Parallel Jacobi: counted, stored and transported over when committed
      record.addLink(interaction);
      return;
    }

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

  /**
  Regular subdivision of a surface element. Parallel Jacobi workers record the
  event (subdivision is idempotent, and the children only depend on the frozen
  state of the parent): the ids of new children are 0 until the commit gives
  them the ids of the serial order
  */
  private static regularSubDivide(element: GalerkinElement, workspace: RefinementWorkspace): void {
    const record = workspace.getRecord();
    if (record === null) {
      element.regularSubDivide();
      return;
    }
    RefinementWorkspace.subdivideUncommitted(element);
    if (element.regularSubElements === null) {
      VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "HierarchicalRefinementStrategy::regularSubDivide", "Element that can not be subdivided in a parallel Jacobi iteration");
    }
    record.addSubdivision(element);
  }

  /**
  Creates the interaction between the receiver and the source (sub-elements of
  a refined interaction), refines it and, if it needs no further refinement,
  stores it. A parallel Jacobi worker may instead leave it to another worker as
  a new task: the FORK event keeps its place in the serial order
  */
  private static refineOrForkSubInteraction(
    scene: Scene,
    candidatesList: Geometry[] | null,
    receiverElement: GalerkinElement,
    sourceElement: GalerkinElement,
    galerkinState: GalerkinState,
    workspace: RefinementWorkspace,
  ): void {
    const scheduler = workspace.getScheduler();
    if (scheduler !== null && scheduler.wantsMoreTasks()) {
      const task = scheduler.fork(receiverElement, sourceElement, candidatesList);
      workspace.getRecord()!.addFork(task);
      return;
    }
    HierarchicalRefinementStrategy.refineSubInteraction(
      scene, candidatesList, receiverElement, sourceElement, galerkinState, workspace,
    );
  }

  private static refineSubInteraction(
    scene: Scene,
    candidatesList: Geometry[] | null,
    receiverElement: GalerkinElement,
    sourceElement: GalerkinElement,
    galerkinState: GalerkinState,
    workspace: RefinementWorkspace,
  ): void {
    const coefficientsPool = workspace.getCoefficientsPool();
    const subInteraction = new Interaction();
    subInteraction.K = HierarchicalRefinementStrategy.borrowKBuffer(coefficientsPool);
    subInteraction.ownsK = false;

    const candidates: Array<Geometry[] | null> = [candidatesList];
    if (HierarchicalRefinementStrategy.hierarchicRefinementCreateSubdivisionLink(
      scene,
      candidates[0] ?? null,
      receiverElement,
      sourceElement,
      subInteraction,
      galerkinState,
      workspace,
    ) !== 0
      && !HierarchicalRefinementStrategy.refineRecursive(scene, candidates, subInteraction, galerkinState, workspace)) {
      HierarchicalRefinementStrategy.hierarchicRefinementStoreInteraction(subInteraction, galerkinState, workspace);
    }

    const borrowed = subInteraction.K;
    subInteraction.K = [];
    HierarchicalRefinementStrategy.returnKBuffer(coefficientsPool, borrowed);
  }

  /**
  An element that could not be regularly subdivided: light transport is done
  over the interaction instead (TypeScript safeguard, not expected)
  */
  private static unsubdividedLightTransport(
    interaction: Interaction,
    galerkinState: GalerkinState,
    workspace: RefinementWorkspace,
  ): void {
    if (workspace.getRecord() !== null) {
      VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "HierarchicalRefinementStrategy::unsubdividedLightTransport", "Element that can not be subdivided in a parallel Jacobi iteration");
    }
    HierarchicalRefinementStrategy.hierarchicRefinementComputeLightTransport(
      interaction, workspace.getScratchVisibilityWorkspace(), galerkinState);
  }

  private static hierarchicRefinementRegularSubdivideSource(
    scene: Scene,
    candidatesList: Array<Geometry[] | null>,
    interaction: Interaction,
    isClusteredGeometry: boolean,
    galerkinState: GalerkinState,
    workspace: RefinementWorkspace,
  ): void {
    const backup = candidatesList[0];
    HierarchicalRefinementStrategy.hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    const sourceElement = interaction.sourceElement;
    const receiverElement = interaction.receiverElement;

    HierarchicalRefinementStrategy.regularSubDivide(sourceElement, workspace);
    if (sourceElement.regularSubElements === null) {
      HierarchicalRefinementStrategy.unsubdividedLightTransport(interaction, galerkinState, workspace);
      return;
    }

    for (let i = 0; i < 4; i++) {
      if (!(sourceElement.regularSubElements[i] instanceof GalerkinElement)) {
        continue;
      }
      const child = sourceElement.regularSubElements[i] as GalerkinElement;
      HierarchicalRefinementStrategy.refineOrForkSubInteraction(
        scene, candidatesList[0] ?? null, receiverElement, child, galerkinState, workspace,
      );
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
    workspace: RefinementWorkspace,
  ): void {
    const backup = candidatesList[0];
    HierarchicalRefinementStrategy.hierarchicRefinementCull(scene, candidatesList, interaction, isClusteredGeometry, galerkinState);
    const sourceElement = interaction.sourceElement;
    const receiverElement = interaction.receiverElement;

    HierarchicalRefinementStrategy.regularSubDivide(receiverElement, workspace);
    if (receiverElement.regularSubElements === null) {
      HierarchicalRefinementStrategy.unsubdividedLightTransport(interaction, galerkinState, workspace);
      return;
    }

    for (let i = 0; i < 4; i++) {
      if (!(receiverElement.regularSubElements[i] instanceof GalerkinElement)) {
        continue;
      }
      const child = receiverElement.regularSubElements[i] as GalerkinElement;
      HierarchicalRefinementStrategy.refineOrForkSubInteraction(
        scene, candidatesList[0] ?? null, child, sourceElement, galerkinState, workspace,
      );
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
    workspace: RefinementWorkspace,
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
            && receiverElement.geometry.getBoundingBox().behindPlane(patch.getNormal(), patch.getPlaneConstant()))
          || (!receiverElement.isCluster() && receiverElement.patch !== null
            && !receiverElement.patch.facing(patch))
        )) {
          continue;
        }
      }
      HierarchicalRefinementStrategy.refineOrForkSubInteraction(
        scene, candidatesList[0] ?? null, receiverElement, childElement, galerkinState, workspace,
      );
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
    workspace: RefinementWorkspace,
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
            && sourceElement.geometry.getBoundingBox().behindPlane(patch.getNormal(), patch.getPlaneConstant()))
          || (!sourceElement.isCluster() && sourceElement.patch !== null
            && !sourceElement.patch.facing(patch))
        )) {
          continue;
        }
      }
      HierarchicalRefinementStrategy.refineOrForkSubInteraction(
        scene, candidatesList[0] ?? null, child, sourceElement, galerkinState, workspace,
      );
    }
    HierarchicalRefinementStrategy.hierarchicRefinementUnCull(candidatesList, galerkinState);
    candidatesList[0] = backup ?? null;
  }

  private static refineRecursive(
    scene: Scene,
    candidatesList: Array<Geometry[] | null>,
    interaction: Interaction,
    galerkinState: GalerkinState,
    workspace: RefinementWorkspace,
  ): boolean {
    let refined: boolean;

    const isClusteredGeometry = (candidatesList[0] === scene.clusteredGeometryList);
    switch (HierarchicalRefinementStrategy.hierarchicRefinementEvaluateInteraction(
      interaction, workspace.getScratchVisibilityWorkspace(), galerkinState)) {
      case InteractionEvaluationCode.ACCURATE_ENOUGH:
        // A parallel Jacobi worker leaves light transport to the commit
        if (workspace.getRecord() === null) {
          HierarchicalRefinementStrategy.hierarchicRefinementComputeLightTransport(
            interaction, workspace.getScratchVisibilityWorkspace(), galerkinState);
        }
        refined = false;
        break;
      case InteractionEvaluationCode.REGULAR_SUBDIVIDE_SOURCE:
        HierarchicalRefinementStrategy.hierarchicRefinementRegularSubdivideSource(
          scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace,
        );
        refined = true;
        break;
      case InteractionEvaluationCode.REGULAR_SUBDIVIDE_RECEIVER:
        HierarchicalRefinementStrategy.hierarchicRefinementRegularSubdivideReceiver(
          scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace,
        );
        refined = true;
        break;
      case InteractionEvaluationCode.SUBDIVIDE_SOURCE_CLUSTER:
        HierarchicalRefinementStrategy.hierarchicRefinementSubdivideSourceCluster(
          scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace,
        );
        refined = true;
        break;
      case InteractionEvaluationCode.SUBDIVIDE_RECEIVER_CLUSTER:
        HierarchicalRefinementStrategy.hierarchicRefinementSubdivideReceiverCluster(
          scene, candidatesList, interaction, isClusteredGeometry, galerkinState, workspace,
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
    workspace: RefinementWorkspace,
  ): boolean {
    let candidateOccluderList: Geometry[] | null = scene.clusteredGeometryList;

    if (galerkinState.exactVisibility !== 0 && interaction.visibility === 255) {
      candidateOccluderList = null;
    }

    const arr: Array<Geometry[] | null> = [candidateOccluderList];
    return HierarchicalRefinementStrategy.refineRecursive(scene, arr, interaction, galerkinState, workspace);
  }

  public static refineInteractions(
    scene: Scene,
    parentElement: GalerkinElement,
    galerkinState: GalerkinState,
  ): void {
    HierarchicalRefinementStrategy.prepareRefinementIterationState(galerkinState);
    const workspace = new RefinementWorkspace(
      scene.getSceneQueryWorkspace(),
      galerkinState.formFactorWorkspace,
      galerkinState.scratchVisibilityWorkspace,
      HierarchicalRefinementStrategy.hierarchicalKPool,
      null,
      null,
    );
    const activePath = new Set<number>();
    HierarchicalRefinementStrategy.refineInteractionsInternal(scene, parentElement, galerkinState, activePath, workspace);
  }

  private static refineInteractionsInternal(
    scene: Scene,
    parentElement: GalerkinElement,
    galerkinState: GalerkinState,
    activePath: Set<number>,
    workspace: RefinementWorkspace,
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
            workspace,
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
              workspace,
            );
          }
        }
      }

      // Refined interactions are removed with order-preserving in-place compaction
      // to avoid one temporary list plus O(n) indexOf/splice scans per element.
      // Only the interactions present when the loop starts are refined: the links
      // that their refinement stores in this same list (the receiver kept, the
      // source subdivided) already had their light transport when created. The
      // original RenderPark prepends them (InteractionListAdd), out of the
      // iteration's way; this list appends them, so they are kept but not
      // visited, or their light would be transported twice in the iteration
      const interactions = parentElement.interactions;
      if (interactions !== null) {
        const numberOfInteractions = interactions.length;
        let writeIndex = 0;
        for (let readIndex = 0; readIndex < interactions.length; readIndex++) {
          const o = interactions[readIndex];
          if (!(o instanceof Interaction)) {
            if (writeIndex !== readIndex) {
              interactions[writeIndex] = o;
            }
            writeIndex++;
            continue;
          }
          const interaction = o;
          if (readIndex < numberOfInteractions
            && HierarchicalRefinementStrategy.refineInteraction(scene, interaction, galerkinState, workspace)) {
            Interaction.interactionDestroy(interaction);
          }
          else {
            if (writeIndex !== readIndex) {
              interactions[writeIndex] = interaction;
            }
            writeIndex++;
          }
        }
        if (writeIndex < interactions.length) {
          interactions.length = writeIndex;
        }
      }
    }
    finally {
      activePath.delete(parentElement.id);
      HierarchicalRefinementStrategy.refineTreeDepth = treeDepth;
    }
  }

  /**
  Parallel Jacobi refinement.
  With Jacobi iterations, refinement only reads state frozen during the
  iteration (radiance, potential, reflectivity, error thresholds), so the
  refinement of each interaction is independent of the others, except for what
  it changes: received radiance and potential (light transport), interaction
  lists (new links), the regular subdivision of elements and their ids and the
  shadow ray counters. The workers refine and compute form factors with their
  own scratch and record those changes; the commit then does them in the order
  of the serial refinement (refineInteractionsInternal), so that every floating
  point sum and every list is the serial one.
  Interactions are those of the element lists when the iteration starts, in the
  serial post-order. Neither the serial refinement nor the commit visits the links
  that an element's own refinement appends to its list (receiver kept, source
  subdivided): their light transport was done when they were created.
  TypeScript: the workers are replicas of the program (see RefinementReplicas),
  and every replica commits all the records, in its own heap.
  */
  private static parallelRefinementSupported(galerkinState: GalerkinState): boolean {
    return (galerkinState.clusteringStrategy === GalerkinClusteringStrategy.ISOTROPIC
      || galerkinState.clusteringStrategy === GalerkinClusteringStrategy.Z_VISIBILITY)
      && galerkinState.shaftCullMode === GalerkinShaftCullMode.NEVER_DO_SHAFT_CULLING
      && galerkinState.multiResolutionVisibility === 0;
  }

  private static numberOfRefinementThreads(galerkinState: GalerkinState): number {
    let threads = galerkinState.numberOfThreads;
    if (threads <= 0) {
      // As VITRAL's ParallelRaytracer.availableProcessors
      const count: number | undefined = typeof navigator !== "undefined" ? navigator.hardwareConcurrency : undefined;
      threads = count !== undefined && count > 0 ? count : 1;
    }
    return threads < 1 ? 1 : globalThis.Math.trunc(threads);
  }

  /**
  Collects, in the order of refineInteractionsInternal, the elements having
  interactions and their interactions
  */
  private static collectRefinementItems(
    parentElement: GalerkinElement,
    groups: RefinementGroup[],
    items: RefinementTask[],
    activePath: Set<number>,
  ): void {
    if (activePath.has(parentElement.id)) {
      return;
    }
    activePath.add(parentElement.id);
    for (let i = 0;
      parentElement.irregularSubElements !== null && i < parentElement.irregularSubElements.length;
      i++) {
      if (parentElement.irregularSubElements[i] instanceof GalerkinElement) {
        HierarchicalRefinementStrategy.collectRefinementItems(
          parentElement.irregularSubElements[i] as GalerkinElement, groups, items, activePath,
        );
      }
    }

    if (parentElement.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        if (parentElement.regularSubElements[i] instanceof GalerkinElement) {
          HierarchicalRefinementStrategy.collectRefinementItems(
            parentElement.regularSubElements[i] as GalerkinElement, groups, items, activePath,
          );
        }
      }
    }
    activePath.delete(parentElement.id);

    const interactions = parentElement.interactions;
    if (interactions !== null && interactions.length > 0) {
      groups.push(new RefinementGroup(parentElement, items.length, interactions.length));
      for (let i = 0; i < interactions.length; i++) {
        items.push(new RefinementTask(interactions[i], null, null, null, items.length));
      }
    }
  }

  /**
  Light transport over the interactions, as if done one after the other in
  their order.
  With a clustering strategy other than isotropic, light transport to a receiver
  cluster writes the surface elements in it, which other receivers (the clusters
  containing it, or contained in it) write too, so it is done serially, with the
  solver's scratch visibility workspace; with Z-visibility, its renderings are
  made ahead (applyClusterLightTransports).
  TypeScript: in one thread (every replica needs all of it in its own heap);
  C++ splits it among its threads by the element written
  */
  private static applyLightTransports(
    transports: Interaction[],
    galerkinState: GalerkinState,
    numberOfThreads: number,
  ): void {
    if (numberOfThreads > 1 && galerkinState.clusteringStrategy === GalerkinClusteringStrategy.Z_VISIBILITY) {
      HierarchicalRefinementStrategy.applyClusterLightTransports(transports, galerkinState, numberOfThreads);
      return;
    }
    const n = transports.length;
    for (let i = 0; i < n; i++) {
      HierarchicalRefinementStrategy.hierarchicRefinementComputeLightTransport(
        transports[i]!, galerkinState.scratchVisibilityWorkspace, galerkinState);
    }
  }

  /**
  Computes ahead, in the plan, what the light transport over the interaction
  takes from the scratch visibility renderer (Z-visibility clustering): the
  radiance of a source cluster and the samples of a receiver cluster seen from
  outside. It reads only state frozen during the iteration
  */
  private static planClusterTransport(
    interaction: Interaction,
    scratch: ScratchVisibilityWorkspace,
    galerkinState: GalerkinState,
    pool: number,
    samplePool: ElementSamples[],
    plan: ClusterTransportPlan,
  ): void {
    if (interaction.sourceElement === interaction.receiverElement) {
      return;
    }
    if (interaction.sourceElement.isCluster()) {
      plan.setSourceRadiance(ClusterTraversalStrategy.sourceClusterRadiance(interaction, scratch, galerkinState));
    }
    const pixelArea = HierarchicalRefinementStrategy.planPixelArea;
    if (interaction.receiverElement.isCluster()
      && ClusterTraversalStrategy.receiverSamples(interaction, scratch, galerkinState, pixelArea)) {
      const visible = scratch.getVisibleElements();
      plan.setReceiverSamples(pixelArea[0]!, pool, samplePool.length, visible.length);
      for (let i = 0; i < visible.length; i++) {
        samplePool.push(visible[i]!);
      }
    }
  }

  // Out parameter of receiverSamples in planClusterTransport, reused
  private static readonly planPixelArea: number[] = [0.0];

  /**
  Light transport over the interactions with Z-visibility clustering, as if
  done one after the other in their order. In batches, the workers first render
  what each interaction needs into their own scratch visibility workspaces
  (planClusterTransport); then the transport sums are done serially, in order,
  from those plans.
  TypeScript: the planning of a batch is done in this thread too, with one
  workspace and one sample pool (every replica needs all of it in its own
  heap), and the plans are reused from one batch to the next; the result is
  the same as C++'s, which plans with its numberOfThreads workers
  */
  private static applyClusterLightTransports(
    transports: Interaction[],
    galerkinState: GalerkinState,
    _numberOfThreads: number,
  ): void {
    const n = transports.length;
    const batchSize = 16384;
    const workspace = new ScratchVisibilityWorkspace(galerkinState.scratchFrameBufferSize);
    // The samples the plans count; cleared, keeping their storage, from one
    // batch to the next
    const samplePools: ElementSamples[][] = [[]];
    const samplePool = samplePools[0]!;
    const plans: ClusterTransportPlan[] = new Array<ClusterTransportPlan>(globalThis.Math.min(batchSize, n));
    for (let i = 0; i < plans.length; i++) {
      plans[i] = new ClusterTransportPlan();
    }
    for (let first = 0; first < n; first += batchSize) {
      const end = globalThis.Math.min(first + batchSize, n);
      samplePool.length = 0;
      for (let i = first; i < end; i++) {
        const plan = plans[i - first]!;
        plan.clear();
        HierarchicalRefinementStrategy.planClusterTransport(transports[i]!, workspace, galerkinState, 0, samplePool, plan);
      }
      for (let i = first; i < end; i++) {
        HierarchicalRefinementStrategy.receiverLightTransport(
          transports[i]!, galerkinState.scratchVisibilityWorkspace, plans[i - first]!, samplePools, galerkinState);
        HierarchicalRefinementStrategy.sourceLightTransport(transports[i]!, galerkinState);
      }
    }
  }

  /**
  Does what the refinement task recorded, in its order: ids for new children,
  counting and storing each new link with its receiver, and the events of the
  tasks it forked, where it forked them. The interactions to transport light
  over (new links, and the interaction itself when it was not refined) are
  appended to transports, in the serial order.
  @return true when the task was a top-level interaction that was refined, so
  that it is to be destroyed
  */
  private static commitRefinementTask(
    scheduler: RefinementScheduler,
    replicas: RefinementReplicas,
    task: RefinementTask,
    elements: Array<GalerkinElement | undefined>,
    transports: Interaction[],
    nextElementId: number[],
  ): boolean {
    const events = replicas.getRecordEvents(task.getWorker());
    const event = new RefinementEvent();

    let offset = task.getFirstEvent();
    const end = task.getEndEvent();
    while (offset < end) {
      offset = event.read(events, offset, elements);
      switch (event.getKind()) {
        case RefinementEvent.SUBDIVISION: {
          const element = event.getReceiver();
          RefinementWorkspace.subdivideUncommitted(element);
          const children = element.regularSubElements!;
          for (let j = 0; j < 4; j++) {
            const child = children[j];
            if (child !== null && child !== undefined && child.id === 0) {
              child.id = nextElementId[0]!;
              nextElementId[0] = nextElementId[0]! + 1;
            }
          }
          break;
        }
        case RefinementEvent.LINK: {
          const link = event.getLink();
          Interaction.countCreated(link);
          transports.push(link);
          if (link.receiverElement.interactions === null) {
            link.receiverElement.interactions = [];
          }
          link.receiverElement.interactions.push(link);
          break;
        }
        case RefinementEvent.FORK:
          HierarchicalRefinementStrategy.commitRefinementTask(
            scheduler, replicas, scheduler.getForked(event.getTask()), elements, transports, nextElementId,
          );
          break;
        default:
          break;
      }
    }

    const interaction = task.getInteraction();
    if (interaction instanceof Interaction && !task.isRefined()) {
      transports.push(interaction);
    }
    return task.isRefined();
  }

  /**
  Same result as refineInteractions with Jacobi iterations, bit for bit, but
  the refinement and form factors of the interactions are computed by
  GalerkinState.numberOfThreads workers, and what they change (light
  transport, new interactions, element ids) is then committed in the serial
  order. Configurations whose refinement reads or writes shared scratch
  (oriented clustering, shaft culling, multi-resolution visibility) are refined
  serially; with Z-visibility clustering each worker renders into its own
  scratch visibility workspace
  */
  public static refineInteractionsInParallel(
    scene: Scene,
    parentElement: GalerkinElement,
    galerkinState: GalerkinState,
  ): void {
    if (!HierarchicalRefinementStrategy.parallelRefinementSupported(galerkinState)) {
      if (galerkinState.iterationNumber <= 1) {
        VsdkLogger.reportMessage(null, VsdkLogger.WARNING, "refineInteractionsInParallel",
          "Parallel Jacobi needs isotropic or Z-visibility clustering, no shaft culling "
          + "and no multi-resolution visibility: refining serially");
      }
      HierarchicalRefinementStrategy.refineInteractions(scene, parentElement, galerkinState);
      return;
    }

    HierarchicalRefinementStrategy.prepareRefinementIterationState(galerkinState);

    // The interactions of the element lists are the items, in the serial order
    const numberOfThreads = HierarchicalRefinementStrategy.numberOfRefinementThreads(galerkinState);
    const groups: RefinementGroup[] = [];
    const items: RefinementTask[] = [];
    HierarchicalRefinementStrategy.collectRefinementItems(parentElement, groups, items, new Set<number>());
    const chunk = globalThis.Math.max(1, globalThis.Math.min(64, globalThis.Math.trunc(items.length / (8 * numberOfThreads))));
    const replicas = RefinementReplicas.forThreads(numberOfThreads);
    const elements = RefinementWorkspace.committedElements(parentElement);
    const scheduler = new RefinementScheduler(replicas, items, chunk, 4 * numberOfThreads, scene, elements);
    replicas.startIteration(items.length, GalerkinElement.getNumberOfElements(), Interaction.getNumberOfInteractions());

    // Phase 1: workers refine the items, and the sub-interactions subdivisions fork
    const firstNewElementId = GalerkinElement.getNumberOfElements() + 1;
    const w = replicas.getWorker();
    const record = new RefinementRecord(replicas.getRecordEvents(w));
    const shadowStatistics = new ShadowStatistics();
    let scratchVisibilityWorkspace: ScratchVisibilityWorkspace | null = null;
    if (galerkinState.clusteringStrategy === GalerkinClusteringStrategy.Z_VISIBILITY) {
      scratchVisibilityWorkspace = new ScratchVisibilityWorkspace(galerkinState.scratchFrameBufferSize);
    }
    const workspace = new RefinementWorkspace(
      scene.getSceneQueryWorkspace(),
      new FormFactorWorkspace(shadowStatistics),
      scratchVisibilityWorkspace,
      HierarchicalRefinementStrategy.hierarchicalKPool,
      record,
      scheduler,
    );
    let failure: unknown = null;
    try {
      const range = [0, 0];
      scheduler.startItems();
      while (scheduler.takeItems(range)) {
        for (let i = range[0]!; i < range[1]!; i++) {
          const task = scheduler.getItem(i);
          const firstEvent = record.getSize();
          const interaction = task.getInteraction();
          const refined = interaction instanceof Interaction
            && HierarchicalRefinementStrategy.refineInteraction(scene, interaction, galerkinState, workspace);
          scheduler.setResult(task, w, firstEvent, record.getSize(), refined);
        }
      }
      scheduler.done();
      for (let task = scheduler.takeForked(); task !== null; task = scheduler.takeForked()) {
        const firstEvent = record.getSize();
        HierarchicalRefinementStrategy.refineSubInteraction(
          scene,
          task.getCandidatesList(),
          task.getReceiver(),
          task.getSource(),
          galerkinState,
          workspace,
        );
        scheduler.setResult(task, w, firstEvent, record.getSize(), false);
        scheduler.done();
      }
    }
    catch (error: unknown) {
      failure = error;
      scheduler.fail();
    }
    if (failure !== null || scheduler.hasFailed()) {
      VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "refineInteractionsInParallel",
        "A refinement worker failed" + (failure instanceof Error ? ": " + failure.message : ""));
    }
    replicas.endRefinement(record.getSize(), shadowStatistics);
    replicas.addShadowStatistics(Statistics.instance().shadow);

    // Phase 2: commit in the serial order; light transport is collected, in
    // that order, and done at the end
    const transports: Interaction[] = [];
    const nextElementId = [firstNewElementId];
    for (let g = 0; g < groups.length; g++) {
      const group = groups[g]!;
      const interactions = group.getElement().interactions;
      let writeIndex = 0;
      for (let readIndex = 0; readIndex < interactions.length; readIndex++) {
        const o = interactions[readIndex];
        // A link appended by this element's refinement (readIndex beyond the
        // items) is kept: its light transport was collected when committed
        let refined = false;
        if (readIndex < group.getNumberOfItems()) {
          refined = HierarchicalRefinementStrategy.commitRefinementTask(
            scheduler,
            replicas,
            scheduler.getItemWithResult(group.getFirstItem() + readIndex),
            elements,
            transports,
            nextElementId,
          );
        }
        if (refined && o instanceof Interaction) {
          Interaction.interactionDestroy(o);
        }
        else {
          if (writeIndex !== readIndex) {
            interactions[writeIndex] = o;
          }
          writeIndex++;
        }
      }
      if (writeIndex < interactions.length) {
        interactions.length = writeIndex;
      }
    }
    HierarchicalRefinementStrategy.applyLightTransports(transports, galerkinState, numberOfThreads);

    if (nextElementId[0] !== GalerkinElement.getNumberOfElements() + 1) {
      VsdkLogger.reportMessage(null, VsdkLogger.FATAL_ERROR, "refineInteractionsInParallel", "Element ids out of the serial order");
    }
  }
}
