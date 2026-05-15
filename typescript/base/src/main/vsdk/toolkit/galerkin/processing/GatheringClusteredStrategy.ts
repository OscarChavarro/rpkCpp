import { RendererConfiguration } from "../../material/RendererConfiguration";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinRadianceMethod } from "../GalerkinRadianceMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinState } from "../GalerkinState";
import { Potential } from "../../render/Potential";
import { Scene } from "../../scene/Scene";
import { ElementFlags } from "../../environment/geometry/elements/ElementFlags";
import { Patch } from "../../environment/geometry/elements/Patch";
import { GatheringStrategy } from "./GatheringStrategy";
import { HierarchicalRefinementStrategy } from "./HierarchicalRefinementStrategy";
import { LinkingClusteredStrategy } from "./LinkingClusteredStrategy";

export class GatheringClusteredStrategy extends GatheringStrategy {
  private static updatePotential(cluster: GalerkinElement): number {
    if ((cluster.flags & ElementFlags.IS_CLUSTER_MASK) !== 0) {
      cluster.potential = 0.0;
      for (let i = 0; cluster.irregularSubElements !== null && i < cluster.irregularSubElements.length; i++) {
        if (!(cluster.irregularSubElements[i] instanceof GalerkinElement)) {
          continue;
        }
        const subCluster = cluster.irregularSubElements[i] as GalerkinElement;
        cluster.potential += subCluster.area * GatheringClusteredStrategy.updatePotential(subCluster);
      }
      cluster.potential /= cluster.area;
    }
    return cluster.potential;
  }

  private static updateClusterDirectPotential(element: GalerkinElement, potentialIncrement: number): void {
    if (element.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        if (element.regularSubElements[i] instanceof GalerkinElement) {
          GatheringClusteredStrategy.updateClusterDirectPotential(
            element.regularSubElements[i] as GalerkinElement,
            potentialIncrement,
          );
        }
      }
    }
    element.directPotential += potentialIncrement;
    element.potential += potentialIncrement;
  }

  public constructor() {
    super();
  }

  public override doGatheringIteration(scene: Scene, galerkinState: GalerkinState, renderOptions: RendererConfiguration): boolean {
    if (galerkinState.importanceDriven !== 0
      && (galerkinState.iterationNumber <= 1 || (scene.camera as NonNullable<Scene["camera"]>).changed !== 0)) {
      Potential.updateDirectPotential(scene, renderOptions);
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        const patch = scene.patchList[i];
        const top = patch.radianceData as GalerkinElement;
        const potentialIncrement = patch.directPotential - top.directPotential;
        GatheringClusteredStrategy.updateClusterDirectPotential(top, potentialIncrement);
      }
      GatheringClusteredStrategy.updatePotential(galerkinState.topCluster as GalerkinElement);
      (scene.camera as NonNullable<Scene["camera"]>).changed = 0;
    }

    process.stdout.write(`Galerkin (clustered) iteration ${galerkinState.iterationNumber}\n`);

    if (galerkinState.iterationNumber <= 1) {
      LinkingClusteredStrategy.createInitialLinks(
        galerkinState.topCluster as GalerkinElement,
        GalerkinRole.RECEIVER,
        galerkinState,
      );
    }

    const userErrorThreshold = galerkinState.relLinkErrorThreshold;
    HierarchicalRefinementStrategy.refineInteractions(scene, galerkinState.topCluster as GalerkinElement, galerkinState);
    galerkinState.relLinkErrorThreshold = userErrorThreshold;

    GalerkinBasis.pushPullRadiance(galerkinState.topCluster, galerkinState);

    if (galerkinState.importanceDriven !== 0) {
      GatheringStrategy.pushPullPotential(galerkinState.topCluster, 0.0);
    }

    galerkinState.ambientRadiance.clear();

    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      GalerkinRadianceMethod.recomputePatchColor(scene.patchList[i]);
    }
    return false;
  }
}
