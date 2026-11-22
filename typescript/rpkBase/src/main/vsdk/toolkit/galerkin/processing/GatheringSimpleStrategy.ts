import { Statistics } from "../../common/statistics/Statistics";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinRadianceMethod } from "../GalerkinRadianceMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinState } from "../GalerkinState";
import { Potential } from "../../render/Potential";
import type { DirectPotentialTable } from "../../scene/DirectPotentialTable";
import type { FaceColorTable } from "../../scene/FaceColorTable";
import { Scene } from "../../scene/Scene";
import { ElementFlags } from "../../environment/geometry/elements/ElementFlags";
import { Patch } from "../../environment/geometry/elements/Patch";
import { GatheringStrategy } from "./GatheringStrategy";
import { HierarchicalRefinementStrategy } from "./HierarchicalRefinementStrategy";
import { LinkingSimpleStrategy } from "./LinkingSimpleStrategy";
import { VSDK } from "vitral/dist/vsdk/toolkit/common/VSDK";

export class GatheringSimpleStrategy extends GatheringStrategy {
  private static patchUpdatePotential(patch: Patch, directPotentials: DirectPotentialTable, galerkinState: GalerkinState): void {
    const topLevelElement = GalerkinElement.fromPatch(galerkinState, patch);
    GatheringStrategy.pushPullPotential(topLevelElement, 0.0, directPotentials);
  }

  /**
  Converts the accumulated received radiance into exitant radiance, making the
  hierarchical representation consistent and computes a new color for the patch
  */
  private static patchUpdateRadiance(patch: Patch, galerkinState: GalerkinState, faceColors: FaceColorTable): void {
    const topLevelElement = GalerkinElement.fromPatch(galerkinState, patch);
    GalerkinBasis.pushPullRadiance(topLevelElement, galerkinState);
    GalerkinRadianceMethod.recomputePatchColor(patch, faceColors);
  }

  private static patchLazyCreateInteractions(
    scene: Scene,
    patch: Patch,
    galerkinState: GalerkinState,
  ): void {
    const topLevelElement = GalerkinElement.fromPatch(galerkinState, patch);
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
    const topLevelElement = GalerkinElement.fromPatch(galerkinState, patch);
    if (topLevelElement === null) {
      return;
    }

    if (galerkinState.importanceDriven !== 0
      && topLevelElement.potential < Statistics.instance().potential.maxDirectPotential * VSDK.EPSILON) {
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
      GatheringSimpleStrategy.patchUpdateRadiance(patch, galerkinState, scene.getFaceColors()!);
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

    if (galerkinState.isJacobi()) {
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        GatheringSimpleStrategy.patchUpdateRadiance(scene.patchList[i]!, galerkinState, scene.getFaceColors()!);
      }
    }

    if (galerkinState.importanceDriven !== 0) {
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        GatheringSimpleStrategy.patchUpdatePotential(scene.patchList[i]!, scene.getDirectPotentials()!, galerkinState);
      }
    }

    return false;
  }
}
