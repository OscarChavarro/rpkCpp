/**
Non diffuse first shot
*/

import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Statistics } from "../../common/statistics/Statistics";
import { Ray } from "../../common/linealAlgebra/Ray";
import { PatchVisitor } from "../../numericalAnalysis/PatchVisitor";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { Scene } from "../../scene/Scene";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Patch } from "../../environment/geometry/elements/Patch";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { Coefficientsmcrad } from "./Coefficientsmcrad";
import { LightSourceTable } from "./LightSourceTable";
import { Localline } from "./Localline";
import { Mcrad } from "./Mcrad";
import { McradP } from "./McradP";
import { Sample4d } from "./Sample4d";
import { StochasticRelaxation } from "./StochasticRelaxation";

export class Nondiff {
  private static lights: LightSourceTable[] | null = null;
  private static numberOfLights = 0;
  private static numberOfSamples = 0;
  private static totalFlux = 0.0;

  private constructor() {
  }

  public static makeLightSourceTable(scenePatches: ArrayList<Patch>, lightPatches: ArrayList<Patch>): void {
    Nondiff.totalFlux = 0.0;
    Nondiff.numberOfLights = Statistics.instance().reader.numberOfLightSources;
    Nondiff.lights = new Array<LightSourceTable>(Nondiff.numberOfLights);
    for (let i = 0; i < Nondiff.numberOfLights; i++) {
      Nondiff.lights[i] = new LightSourceTable();
    }

    for (let i = 0; lightPatches !== null && i < lightPatches.size(); i++) {
      const light = lightPatches.get(i)!;
      const emittedRadiance = PatchVisitor.averageEmittance(light, 0x01 | 0x02 | 0x04);
      const flux = globalThis.Math.PI * light.area * emittedRadiance.sumAbsComponents();
      Nondiff.totalFlux += flux;
      Nondiff.lights[i] = new LightSourceTable(light, flux);
    }

    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      const patch = scenePatches.get(i)!;
      Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchRad(patch), McradP.getTopLevelPatchBasis(patch));
      Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchUnShotRad(patch), McradP.getTopLevelPatchBasis(patch));
      Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));
      McradP.topLevelStochasticRadiosityElement(patch).sourceRad.clear();
    }
  }

  public static nextLightSample(patch: Patch, zeta: number[]): void {
    const xi = Sample4d.sample4D(McradP.topLevelStochasticRadiosityElement(patch).rayIndex | 0);
    McradP.topLevelStochasticRadiosityElement(patch).rayIndex++;
    if (patch.numberOfVertices === 3) {
      const u = [xi[0]!];
      const v = [xi[1]!];
      Sample4d.foldSampleF(u, v);
      zeta[0] = u[0]!;
      zeta[1] = v[0]!;
    }
    else {
      zeta[0] = xi[0]!;
      zeta[1] = xi[1]!;
    }
    zeta[2] = xi[2]!;
    zeta[3] = xi[3]!;
  }

  public static sampleLightRay(
    patch: Patch,
    emittedRad: ColorRgb,
    pointSelectionPdf: number[],
    dirSelectionPdf: number[]
  ): Ray {
    const ray = new Ray();
    do {
      const zeta = new Array<number>(4);
      const hit = new RayHit();
      Nondiff.nextLightSample(patch, zeta);

      patch.uniformPoint(zeta[0]!, zeta[1]!, ray.position);

      hit.init(patch, ray.position, patch.normal, patch.material);
      dirSelectionPdf[0] = 0.0;
      ray.direction.set(0.0, 0.0, 0.0);
      if (patch.material !== null && patch.material.getEdf() !== null) {
        const shctxOk = [false];
        const shctx = hit.shadingContext(shctxOk);
        if (!shctxOk[0]) {
          continue;
        }
        ray.direction = patch.material.getEdf()!.phongEdfSample(
          shctx,
          0x01 | 0x02 | 0x04,
          zeta[2]!,
          zeta[3]!,
          emittedRad,
          dirSelectionPdf
        );
      }
    } while (dirSelectionPdf[0] === 0.0);

    pointSelectionPdf[0] = 1.0 / patch.area;
    return ray;
  }

  public static sampleLight(sceneWorldVoxelGrid: VoxelGrid, light: LightSourceTable, lightSelectionPdf: number): void {
    const rad = new ColorRgb();
    const pointSelectionPdf = [0.0];
    const dirSelectionPdf = [0.0];
    const lightPatch = light.patch as Patch;
    const ray = Nondiff.sampleLightRay(lightPatch, rad, pointSelectionPdf, dirSelectionPdf);
    const hitStore = new RayHit();

    StochasticRelaxation.activeState().tracedRays++;
    const hit = Localline.mcrShootRay(sceneWorldVoxelGrid, lightPatch, ray, hitStore);
    if (hit !== null) {
      const hitPatch = hit.getPatch() as Patch;
      const topPatchRad = McradP.getTopLevelPatchRad(hitPatch)!;
      const topPatchUnShot = McradP.getTopLevelPatchUnShotRad(hitPatch)!;
      const pdf = lightSelectionPdf * pointSelectionPdf[0]! * dirSelectionPdf[0]!;
      const outCos = ray.direction.dotProduct(lightPatch.normal);
      const receivedRadiosity = new ColorRgb();
      const rd = McradP.topLevelStochasticRadiosityElement(hitPatch).Rd;
      receivedRadiosity.scaledCopy(outCos / (globalThis.Math.PI * hitPatch.area * pdf * Nondiff.numberOfSamples), rad);
      receivedRadiosity.selfScalarProduct(rd);
      topPatchRad[0]!.add(topPatchRad[0]!, receivedRadiosity);
      topPatchUnShot[0]!.add(topPatchUnShot[0]!, receivedRadiosity);
      McradP.topLevelStochasticRadiosityElement(hitPatch).sourceRad.add(
        McradP.topLevelStochasticRadiosityElement(hitPatch).sourceRad,
        receivedRadiosity
      );
    }
  }

  public static sampleLightSources(sceneWorldVoxelGrid: VoxelGrid, samplesCount: number): void {
    let rnd = globalThis.Math.random();
    let count = 0;
    let pCumulative = 0.0;
    Nondiff.numberOfSamples = samplesCount;
    process.stderr.write(`Shooting ${Nondiff.numberOfSamples} light rays `);
    const lights = Nondiff.lights as LightSourceTable[];
    for (let i = 0; i < Nondiff.numberOfLights; i++) {
      const p = lights[i]!.flux / Nondiff.totalFlux;
      const samplesThisLight =
        globalThis.Math.floor((pCumulative + p) * Nondiff.numberOfSamples + rnd) - count;

      for (let j = 0; j < samplesThisLight; j++) {
        Nondiff.sampleLight(sceneWorldVoxelGrid, lights[i]!, p);
      }

      pCumulative += p;
      count += samplesThisLight;
    }

    process.stderr.write("\n");
  }

  public static summarize(scenePatches: ArrayList<Patch>): void {
    StochasticRelaxation.activeState().unShotFlux.clear();
    StochasticRelaxation.activeState().unShotYmp = 0.0;
    StochasticRelaxation.activeState().totalFlux.clear();
    StochasticRelaxation.activeState().totalYmp = 0.0;
    StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.clear();
    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      const patch = scenePatches.get(i)!;
      StochasticRelaxation.activeState().unShotFlux.addScaled(
        StochasticRelaxation.activeState().unShotFlux,
        globalThis.Math.PI * patch.area,
        McradP.getTopLevelPatchUnShotRad(patch)![0]!
      );
      StochasticRelaxation.activeState().totalFlux.addScaled(
        StochasticRelaxation.activeState().totalFlux,
        globalThis.Math.PI * patch.area,
        McradP.getTopLevelPatchRad(patch)![0]!
      );
      StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.addScaled(
        StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux,
        globalThis.Math.PI * patch.area * (McradP.topLevelStochasticRadiosityElement(patch).importance -
          McradP.topLevelStochasticRadiosityElement(patch).sourceImportance),
        McradP.getTopLevelPatchUnShotRad(patch)![0]!
      );
      StochasticRelaxation.activeState().unShotYmp += patch.area * globalThis.Math.abs(
        McradP.topLevelStochasticRadiosityElement(patch).unShotImportance
      );
      StochasticRelaxation.activeState().totalYmp += patch.area * McradP.topLevelStochasticRadiosityElement(patch).importance;
      StochasticRelaxation.activeState().sourceYmp += patch.area * McradP.topLevelStochasticRadiosityElement(patch).sourceImportance;
      Mcrad.monteCarloRadiosityPatchComputeNewColor(patch);
    }
  }

  /**
Initial shooting pass handling non-diffuse light sources
*/
  public static doNonDiffuseFirstShot(
    scene: Scene,
    radianceMethod: RadianceMethod,
    renderOptions: RendererConfiguration
  ): void {
    void radianceMethod;
    void renderOptions;

    const scenePatches = new ArrayList<Patch>();
    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      scenePatches.add(scene.patchList[i]!);
    }

    const lightPatches = new ArrayList<Patch>();
    for (let i = 0; scene.lightSourcePatchList !== null && i < scene.lightSourcePatchList.length; i++) {
      lightPatches.add(scene.lightSourcePatchList[i]!);
    }

    Nondiff.makeLightSourceTable(scenePatches, lightPatches);
    Nondiff.sampleLightSources(
      scene.voxelGrid as VoxelGrid,
      StochasticRelaxation.activeState().initialLightSourceSamples * Nondiff.numberOfLights
    );
    Nondiff.summarize(scenePatches);
    Nondiff.lights = null;
  }
}
