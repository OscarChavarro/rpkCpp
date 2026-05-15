import { ColorRgb } from "../../common/color/ColorRgb";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Statistics } from "../../common/statistics/Statistics";
import { GalerkinBasis } from "../GalerkinBasis";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinRadianceMethod } from "../GalerkinRadianceMethod";
import { GalerkinRole } from "../GalerkinRole";
import { GalerkinState } from "../GalerkinState";
import { Potential } from "../../render/Potential";
import { Scene } from "../../scene/Scene";
import { Element } from "../../environment/geometry/elements/Element";
import { ElementFlags } from "../../environment/geometry/elements/ElementFlags";
import { Patch } from "../../environment/geometry/elements/Patch";
import { HierarchicalRefinementStrategy } from "./HierarchicalRefinementStrategy";
import { LinkingClusteredStrategy } from "./LinkingClusteredStrategy";
import { LinkingSimpleStrategy } from "./LinkingSimpleStrategy";

export class ShootingStrategy {
  private static galerkinGetPotential(patch: Patch): number {
    return (patch !== null && patch.radianceData instanceof GalerkinElement)
      ? (patch.radianceData as GalerkinElement).potential : 0.0;
  }

  private static galerkinGetUnShotPotential(patch: Patch): number {
    return (patch !== null && patch.radianceData instanceof GalerkinElement)
      ? (patch.radianceData as GalerkinElement).unShotPotential : 0.0;
  }

  private static chooseRadianceShootingPatch(scenePatches: Patch[] | null, galerkinState: GalerkinState): Patch | null {
    let shootingPatch: Patch | null = null;
    let potentialShootingPatch: Patch | null = null;
    let maximumPower = 0.0;
    let maximumPowerImportance = 0.0;

    for (let i = 0; scenePatches !== null && i < scenePatches.length; i++) {
      const patch = scenePatches[i];
      if (patch === null || patch.radianceData === null || patch.radianceData.unShotRadiance === null
        || patch.radianceData.unShotRadiance.length === 0 || patch.radianceData.unShotRadiance[0] === null) {
        continue;
      }

      const power = globalThis.Math.PI * patch.area * patch.radianceData.unShotRadiance[0].sumAbsComponents();
      if (power > maximumPower) {
        shootingPatch = patch;
        maximumPower = power;
      }

      if (galerkinState.importanceDriven !== 0) {
        const powerImportance = (ShootingStrategy.galerkinGetPotential(patch) - patch.directPotential) * power;
        if (powerImportance > maximumPowerImportance) {
          potentialShootingPatch = patch;
          maximumPowerImportance = powerImportance;
        }
      }
    }

    if (galerkinState.importanceDriven !== 0 && potentialShootingPatch !== null) {
      return potentialShootingPatch;
    }
    return shootingPatch;
  }

  private static clearUnShotRadianceAndPotential(elem: GalerkinElement): void {
    if (elem === null) {
      return;
    }

    if (elem.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        const child = elem.regularSubElements[i];
        if (child instanceof GalerkinElement) {
          ShootingStrategy.clearUnShotRadianceAndPotential(child as GalerkinElement);
        }
      }
    }

    for (let i = 0; elem.irregularSubElements !== null && i < elem.irregularSubElements.length; i++) {
      const child = elem.irregularSubElements[i];
      if (child instanceof GalerkinElement) {
        ShootingStrategy.clearUnShotRadianceAndPotential(child as GalerkinElement);
      }
    }

    if (elem.unShotRadiance !== null) {
      ColorRgb.arrayClear(elem.unShotRadiance as ColorRgb[], elem.basisSize);
    }
    elem.unShotPotential = 0.0;
  }

  private static patchPropagateUnShotRadianceAndPotential(
    scene: Scene,
    patch: Patch,
    galerkinState: GalerkinState,
  ): void {
    if (scene === null || patch === null || galerkinState === null) {
      return;
    }

    const topLevelElement = GalerkinElement.fromPatch(patch);
    if (topLevelElement === null) {
      return;
    }

    if ((topLevelElement.flags & ElementFlags.INTERACTIONS_CREATED_MASK) === 0) {
      if (galerkinState.clustered !== 0) {
        LinkingClusteredStrategy.createInitialLinks(topLevelElement, GalerkinRole.SOURCE, galerkinState);
      }
      else {
        LinkingSimpleStrategy.createInitialLinks(
          scene,
          galerkinState,
          GalerkinRole.SOURCE,
          topLevelElement,
        );
      }
      topLevelElement.flags |= ElementFlags.INTERACTIONS_CREATED_MASK;
    }

    HierarchicalRefinementStrategy.refineInteractions(scene, topLevelElement, galerkinState);

    ShootingStrategy.clearUnShotRadianceAndPotential(topLevelElement);
  }

  private static shootingPushPullPotential(element: GalerkinElement, down: number): number {
    if (element === null) {
      return 0.0;
    }

    if (element.area !== 0.0) {
      down += element.receivedPotential / element.area;
    }
    element.receivedPotential = 0.0;

    let up = 0.0;

    if (element.regularSubElements === null
      && (element.irregularSubElements === null || element.irregularSubElements.length === 0)) {
      up = down;
    }

    if (element.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        const child = element.regularSubElements[i];
        if (child instanceof GalerkinElement) {
          up += 0.25 * ShootingStrategy.shootingPushPullPotential(child as GalerkinElement, down);
        }
      }
    }

    if (element.irregularSubElements !== null) {
      for (let j = 0; j < element.irregularSubElements.length; j++) {
        const child = element.irregularSubElements[j];
        if (!(child instanceof GalerkinElement)) {
          continue;
        }
        const subElement = child as GalerkinElement;
        if (!element.isCluster()) {
          down = 0.0;
        }
        if (element.area !== 0.0) {
          up += subElement.area / element.area * ShootingStrategy.shootingPushPullPotential(subElement, down);
        }
      }
    }

    element.potential += up;
    element.unShotPotential += up;
    return up;
  }

  private static patchUpdateRadianceAndPotential(patch: Patch, galerkinState: GalerkinState): void {
    if (patch === null || galerkinState === null) {
      return;
    }

    const topLevelElement = GalerkinElement.fromPatch(patch);
    if (topLevelElement === null) {
      return;
    }

    if (galerkinState.importanceDriven !== 0) {
      ShootingStrategy.shootingPushPullPotential(topLevelElement, 0.0);
    }
    GalerkinBasis.pushPullRadiance(topLevelElement, galerkinState);

    if (patch.radianceData !== null && patch.radianceData.unShotRadiance !== null
      && patch.radianceData.unShotRadiance.length > 0 && patch.radianceData.unShotRadiance[0] !== null) {
      galerkinState.ambientRadiance.addScaled(
        galerkinState.ambientRadiance,
        patch.area,
        patch.radianceData.unShotRadiance[0],
      );
    }
  }

  private static doPropagate(scene: Scene, shootingPatch: Patch, galerkinState: GalerkinState): void {
    if (scene === null || galerkinState === null || shootingPatch === null) {
      return;
    }

    ShootingStrategy.patchPropagateUnShotRadianceAndPotential(scene, shootingPatch, galerkinState);

    if (galerkinState.clustered !== 0) {
      if (galerkinState.importanceDriven !== 0) {
        ShootingStrategy.shootingPushPullPotential(galerkinState.topCluster as GalerkinElement, 0.0);
      }
      GalerkinBasis.pushPullRadiance(galerkinState.topCluster, galerkinState);
      if (galerkinState.topCluster !== null
        && galerkinState.topCluster.unShotRadiance !== null
        && galerkinState.topCluster.unShotRadiance.length > 0
        && galerkinState.topCluster.unShotRadiance[0] !== null) {
        const ambient = galerkinState.topCluster.unShotRadiance[0];
        galerkinState.ambientRadiance.set(ambient.r, ambient.g, ambient.b);
      }
    }
    else {
      galerkinState.ambientRadiance.clear();
      for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
        ShootingStrategy.patchUpdateRadianceAndPotential(scene.patchList[i], galerkinState);
      }
      galerkinState.ambientRadiance.scale(1.0 / Statistics.instance().radiance.totalArea);
    }

    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      GalerkinRadianceMethod.recomputePatchColor(scene.patchList[i]);
    }
  }

  private static propagateRadiance(scene: Scene, galerkinState: GalerkinState): boolean {
    const shootingPatch = ShootingStrategy.chooseRadianceShootingPatch(scene.patchList, galerkinState);
    if (shootingPatch === null) {
      return true;
    }

    ShootingStrategy.doPropagate(scene, shootingPatch, galerkinState);
    return false;
  }

  private static clusterUpdatePotential(clusterElement: GalerkinElement): void {
    if (clusterElement === null || !clusterElement.isCluster()) {
      return;
    }

    clusterElement.potential = 0.0;
    clusterElement.unShotPotential = 0.0;
    for (let i = 0; clusterElement.irregularSubElements !== null
      && i < clusterElement.irregularSubElements.length; i++) {
      const child = clusterElement.irregularSubElements[i];
      if (!(child instanceof GalerkinElement)) {
        continue;
      }
      const subCluster = child as GalerkinElement;
      ShootingStrategy.clusterUpdatePotential(subCluster);
      clusterElement.potential += subCluster.area * subCluster.potential;
      clusterElement.unShotPotential += subCluster.area * subCluster.unShotPotential;
    }
    if (clusterElement.area !== 0.0) {
      clusterElement.potential /= clusterElement.area;
      clusterElement.unShotPotential /= clusterElement.area;
    }
  }

  private static choosePotentialShootingPatch(scenePatches: Patch[] | null): Patch | null {
    let maximumImportance = 0.0;
    let shootingPatch: Patch | null = null;

    for (let i = 0; scenePatches !== null && i < scenePatches.length; i++) {
      const patch = scenePatches[i];
      if (patch === null) {
        continue;
      }
      const importance = patch.area * globalThis.Math.abs(ShootingStrategy.galerkinGetUnShotPotential(patch));
      if (importance > maximumImportance) {
        shootingPatch = patch;
        maximumImportance = importance;
      }
    }

    return shootingPatch;
  }

  private static propagatePotential(scene: Scene, galerkinState: GalerkinState): void {
    const shootingPatch = ShootingStrategy.choosePotentialShootingPatch(scene.patchList);
    if (shootingPatch !== null) {
      ShootingStrategy.doPropagate(scene, shootingPatch, galerkinState);
    }
    else {
      process.stderr.write("No patches with un-shot potential??\n");
    }
  }

  private static shootingUpdateDirectPotential(galerkinElement: GalerkinElement, potentialIncrement: number): void {
    if (galerkinElement === null) {
      return;
    }

    if (galerkinElement.regularSubElements !== null) {
      for (let i = 0; i < 4; i++) {
        const child = galerkinElement.regularSubElements[i];
        if (child instanceof GalerkinElement) {
          ShootingStrategy.shootingUpdateDirectPotential(child as GalerkinElement, potentialIncrement);
        }
      }
    }
    galerkinElement.directPotential += potentialIncrement;
    galerkinElement.potential += potentialIncrement;
    galerkinElement.unShotPotential += potentialIncrement;
  }

  public static doShootingStep(scene: Scene, galerkinState: GalerkinState, renderOptions: RendererConfiguration): boolean {
    if (galerkinState.importanceDriven !== 0) {
      if (galerkinState.iterationNumber <= 1 || (scene.camera as NonNullable<Scene["camera"]>).changed !== 0) {
        Potential.updateDirectPotential(scene, renderOptions);
        for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
          const patch = scene.patchList[i];
          const topLevelElement = GalerkinElement.fromPatch(patch);
          if (topLevelElement === null) {
            continue;
          }
          const potentialIncrement = patch.directPotential - topLevelElement.directPotential;
          ShootingStrategy.shootingUpdateDirectPotential(topLevelElement, potentialIncrement);
        }
        (scene.camera as NonNullable<Scene["camera"]>).changed = 0;
        if (galerkinState.clustered !== 0) {
          ShootingStrategy.clusterUpdatePotential(galerkinState.topCluster as GalerkinElement);
        }
      }
      ShootingStrategy.propagatePotential(scene, galerkinState);
    }
    return ShootingStrategy.propagateRadiance(scene, galerkinState);
  }
}
