import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Statistics } from "../../common/statistics/Statistics";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinRadianceMethod } from "../GalerkinRadianceMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinState } from "../GalerkinState";
import { Potential } from "../../render/Potential";
import { Scene } from "../../scene/Scene";
import { ElementFlags } from "../../environment/geometry/elements/ElementFlags";
import { Patch } from "../../environment/geometry/elements/Patch";
import { GatheringStrategy } from "./GatheringStrategy";
import { HierarchicalRefinementStrategy } from "./HierarchicalRefinementStrategy";
import { LinkingSimpleStrategy } from "./LinkingSimpleStrategy";

export class GatheringSimpleStrategy extends GatheringStrategy {
  private static patchUpdatePotential(patch: Patch): void {
    const topLevelElement = GalerkinElement.fromPatch(patch);
    GatheringStrategy.pushPullPotential(topLevelElement, 0.0);
  }

  private static patchUpdateRadiance(patch: Patch, galerkinState: GalerkinState): void {
    const topLevelElement = GalerkinElement.fromPatch(patch);
    GalerkinBasis.pushPullRadiance(topLevelElement, galerkinState);
    GalerkinRadianceMethod.recomputePatchColor(patch);
  }

  private static patchLazyCreateInteractions(
    scene: Scene,
    patch: Patch,
    galerkinState: GalerkinState,
  ): void {
    const topLevelElement = GalerkinElement.fromPatch(patch);
    if (topLevelElement === null) {
      return;
    }

    if (!(topLevelElement.radiance as NonNullable<GalerkinElement["radiance"]>)[0]!.isBlack()
      && (topLevelElement.flags & ElementFlags.INTERACTIONS_CREATED_MASK) === 0) {
      LinkingSimpleStrategy.createInitialLinks(
        scene,
        galerkinState,
        GalerkinRole.SOURCE,
        topLevelElement,
      );
      topLevelElement.flags |= ElementFlags.INTERACTIONS_CREATED_MASK;
    }
  }

  private static patchGather(
    patch: Patch,
    scene: Scene,
    galerkinState: GalerkinState,
  ): void {
    const topLevelElement = GalerkinElement.fromPatch(patch);
    if (topLevelElement === null) {
      return;
    }

    if (galerkinState.importanceDriven !== 0
      && topLevelElement.potential < Statistics.instance().potential.maxDirectPotential * Numeric.EPSILON) {
      return;
    }

    if ((galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL
        || galerkinState.lazyLinking === 0
        || galerkinState.importanceDriven !== 0)
      && (topLevelElement.flags & ElementFlags.INTERACTIONS_CREATED_MASK) === 0) {
      LinkingSimpleStrategy.createInitialLinks(
        scene,
        galerkinState,
        GalerkinRole.RECEIVER,
        topLevelElement,
      );
      topLevelElement.flags |= ElementFlags.INTERACTIONS_CREATED_MASK;
    }

    HierarchicalRefinementStrategy.refineInteractions(scene, topLevelElement, galerkinState);

    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.GAUSS_SEIDEL) {
      GatheringSimpleStrategy.patchUpdateRadiance(patch, galerkinState);
    }
  }

  public constructor() {
    super();
  }

  public override doGatheringIteration(scene: Scene, galerkinState: GalerkinState, renderOptions: RendererConfiguration): boolean {
    if (galerkinState.importanceDriven !== 0
      && (galerkinState.iterationNumber <= 1 || (scene.camera as NonNullable<Scene["camera"]>).changed !== 0)) {
      Potential.updateDirectPotential(scene, renderOptions);
      (scene.camera as NonNullable<Scene["camera"]>).changed = 0;
    }

    if (galerkinState.galerkinIterationMethod !== GalerkinIterationMethod.GAUSS_SEIDEL
      && galerkinState.lazyLinking !== 0
      && galerkinState.importanceDriven === 0) {
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        GatheringSimpleStrategy.patchLazyCreateInteractions(scene, scene.patchList[i]!, galerkinState);
      }
    }

    galerkinState.ambientRadiance.clear();

    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      GatheringSimpleStrategy.patchGather(scene.patchList[i]!, scene, galerkinState);
    }

    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.JACOBI) {
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        GatheringSimpleStrategy.patchUpdateRadiance(scene.patchList[i]!, galerkinState);
      }
    }

    if (galerkinState.importanceDriven !== 0) {
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        GatheringSimpleStrategy.patchUpdatePotential(scene.patchList[i]!);
      }
    }

    return false;
  }
}
