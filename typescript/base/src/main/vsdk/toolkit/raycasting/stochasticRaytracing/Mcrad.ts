/**
Monte Carlo radiosity
*/

import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { BsdfComponent } from "../../material/BsdfComponent";
import { RayHitFlag } from "../../environment/geometry/elements/RayHitFlag";
import { PatchVisitor } from "../../numericalAnalysis/PatchVisitor";
import { Potential } from "../../render/Potential";
import { Scene } from "../../scene/Scene";
import { Element } from "../../environment/geometry/elements/Element";
import { ElementTypes } from "../../environment/geometry/elements/ElementTypes";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../environment/geometry/elements/Patch";
import { RayHit } from "../../environment/geometry/elements/RayHit";
import { Vertex } from "../../environment/geometry/elements/Vertex";
import { Basismcrad } from "./Basismcrad";
import { Coefficientsmcrad } from "./Coefficientsmcrad";
import { ElementHierarchyState } from "./ElementHierarchyState";
import { Hierarchy } from "./Hierarchy";
import { McradP } from "./McradP";
import { RandomWalkEstimatorKind } from "./RandomWalkEstimatorKind";
import { RandomWalkEstimatorType } from "./RandomWalkEstimatorType";
import { Sample4d } from "./Sample4d";
import { Sampler4DSequence } from "./Sampler4DSequence";
import { StochasticRadiosityElement } from "./StochasticRadiosityElement";
import { StochasticRelaxation } from "./StochasticRelaxation";
import { StochasticRaytracingApproximation } from "./StochasticRaytracingApproximation";
import { WhatToShow } from "./WhatToShow";

/**
Common routines for stochastic relaxation and random walks
*/
export class Mcrad {
  private constructor() {
  }

  public static monteCarloRadiosityDefaults(): void {
    StochasticRelaxation.activeState().inited = 0;
    StochasticRelaxation.activeState().rayUnitsPerIt = 10;
    StochasticRelaxation.activeState().bidirectionalTransfers = 0;
    StochasticRelaxation.activeState().constantControlVariate = 0;
    StochasticRelaxation.activeState().controlRadiance.clear();
    StochasticRelaxation.activeState().indirectOnly = 0;
    StochasticRelaxation.activeState().sequence = Sampler4DSequence.NIEDERREITER;
    StochasticRelaxation.activeState().approximationOrderType = StochasticRaytracingApproximation.CONSTANT;
    StochasticRelaxation.activeState().importanceDriven = 0;
    StochasticRelaxation.activeState().radianceDriven = 1;
    StochasticRelaxation.activeState().importanceUpdated = 0;
    StochasticRelaxation.activeState().importanceUpdatedFromScratch = 0;
    StochasticRelaxation.activeState().continuousRandomWalk = 0;
    StochasticRelaxation.activeState().randomWalkEstimatorType = RandomWalkEstimatorType.RW_SHOOTING;
    StochasticRelaxation.activeState().randomWalkEstimatorKind = RandomWalkEstimatorKind.RW_COLLISION;
    StochasticRelaxation.activeState().randomWalkNumLast = 1;
    StochasticRelaxation.activeState().weightedSampling = 0;
    StochasticRelaxation.activeState().discardIncremental = 0;
    StochasticRelaxation.activeState().incrementalUsesImportance = 0;
    StochasticRelaxation.activeState().naiveMerging = 0;
    StochasticRelaxation.activeState().show = WhatToShow.SHOW_TOTAL_RADIANCE;
    StochasticRelaxation.activeState().doNonDiffuseFirstShot = 0;
    StochasticRelaxation.activeState().initialLightSourceSamples = 1000;

    Hierarchy.elementHierarchyDefaults();
    Basismcrad.monteCarloRadiosityInitBasis();
  }

  /**
For counting how much CPU time was used for the computations
*/
  public static monteCarloRadiosityUpdateCpuSecs(): void {
    const t = Number(process.hrtime.bigint());
    StochasticRelaxation.activeState().cpuSeconds +=
      (t - StochasticRelaxation.activeState().lastClock) / 1_000_000_000.0;
    StochasticRelaxation.activeState().lastClock = t;
  }

  public static monteCarloRadiosityCreatePatchData(patch: Patch): Element {
    patch.radianceData = StochasticRadiosityElement.stochasticRadiosityElementCreateFromPatch(patch);
    return patch.radianceData;
  }

  public static monteCarloRadiosityDestroyPatchData(patch: Patch): void {
    if (patch.radianceData !== null) {
      StochasticRadiosityElement.stochasticRadiosityElementDestroy(McradP.topLevelStochasticRadiosityElement(patch));
    }
    patch.radianceData = null;
  }

  /**
Compute new color for the patch: fine if no hierarchical refinement is used, e.g.
in the current random walk radiosity implementation
*/
  public static monteCarloRadiosityPatchComputeNewColor(patch: Patch): void {
    patch.color = StochasticRadiosityElement.stochasticRadiosityElementColor(McradP.topLevelStochasticRadiosityElement(patch));
    patch.computeVertexColors();
  }

  /**
Initializes the computations for the current scene (if any): initialisations
are delayed to just before the first iteration step, see ReInit() below
*/
  public static monteCarloRadiosityInit(): void {
    StochasticRelaxation.activeState().inited = 0;
  }

  /**
Initialises patch data
*/
  private static monteCarloRadiosityInitPatch(patch: Patch): void {
    const Ed = McradP.topLevelStochasticRadiosityElement(patch).Ed;
    const patchRad = McradP.getTopLevelPatchRad(patch);
    const patchUnShotRad = McradP.getTopLevelPatchUnShotRad(patch);
    const patchReceivedRad = McradP.getTopLevelPatchReceivedRad(patch);

    Coefficientsmcrad.reAllocCoefficients(McradP.topLevelStochasticRadiosityElement(patch));
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(patchRad, McradP.getTopLevelPatchBasis(patch));
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(patchUnShotRad, McradP.getTopLevelPatchBasis(patch));
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(patchReceivedRad, McradP.getTopLevelPatchBasis(patch));

    patchRad![0] = new ColorRgb(Ed.r, Ed.g, Ed.b);
    patchUnShotRad![0] = new ColorRgb(Ed.r, Ed.g, Ed.b);
    McradP.topLevelStochasticRadiosityElement(patch).sourceRad = new ColorRgb(Ed.r, Ed.g, Ed.b);
    patchReceivedRad![0]!.clear();

    McradP.topLevelStochasticRadiosityElement(patch).rayIndex = patch.id * 11;
    McradP.topLevelStochasticRadiosityElement(patch).quality = 0.0;
    McradP.topLevelStochasticRadiosityElement(patch).ng = 0;
    McradP.topLevelStochasticRadiosityElement(patch).importance = 0.0;
    McradP.topLevelStochasticRadiosityElement(patch).unShotImportance = 0.0;
    McradP.topLevelStochasticRadiosityElement(patch).receivedImportance = 0.0;
    McradP.topLevelStochasticRadiosityElement(patch).sourceImportance = 0.0;
  }

  /**
Routines below update/re-initialise importance after a viewing change
*/
  private static monteCarloRadiosityPullImportances(element: Element): void {
    const child = element as StochasticRadiosityElement;
    const parent = child.parent as StochasticRadiosityElement;
    const scale = child.area / parent.area;
    parent.importance += scale * child.importance;
    parent.sourceImportance += scale * child.sourceImportance;
    parent.unShotImportance += scale * child.unShotImportance;
  }

  private static monteCarloRadiosityAccumulateImportances(elem: StochasticRadiosityElement): void {
    StochasticRelaxation.activeState().totalYmp += elem.area * elem.importance;
    StochasticRelaxation.activeState().sourceYmp += elem.area * elem.sourceImportance;
    StochasticRelaxation.activeState().unShotYmp += elem.area * globalThis.Math.abs(elem.unShotImportance);
  }

  /**
Update importance in the element hierarchy starting with the top cluster
*/
  private static monteCarloRadiosityUpdateImportance(element: Element): void {
    const stochasticRadiosityElement = element as StochasticRadiosityElement;

    if (stochasticRadiosityElement === null) {
      return;
    }

    if (!stochasticRadiosityElement.traverseAllChildren((child) => Mcrad.monteCarloRadiosityUpdateImportance(child))) {
      const deltaImp = (stochasticRadiosityElement.patch !== null && stochasticRadiosityElement.patch.isVisible() ? 1.0 : 0.0)
        - stochasticRadiosityElement.sourceImportance;
      stochasticRadiosityElement.importance += deltaImp;
      stochasticRadiosityElement.sourceImportance += deltaImp;
      stochasticRadiosityElement.unShotImportance += deltaImp;
      Mcrad.monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
    }
    else {
      stochasticRadiosityElement.importance = 0.0;
      stochasticRadiosityElement.sourceImportance = 0.0;
      stochasticRadiosityElement.unShotImportance = 0.0;
      stochasticRadiosityElement.traverseAllChildren((child) => Mcrad.monteCarloRadiosityPullImportances(child));
    }
  }

  /**
Re-init importance in the element hierarchy starting with the top cluster
*/
  private static monteCarloRadiosityReInitImportance(element: Element): void {
    const stochasticRadiosityElement = element as StochasticRadiosityElement;

    if (stochasticRadiosityElement === null) {
      return;
    }

    if (!stochasticRadiosityElement.traverseAllChildren((child) => Mcrad.monteCarloRadiosityReInitImportance(child))) {
      stochasticRadiosityElement.importance =
        (stochasticRadiosityElement.patch !== null && stochasticRadiosityElement.patch.isVisible() ? 1.0 : 0.0);
      stochasticRadiosityElement.sourceImportance = stochasticRadiosityElement.importance;
      stochasticRadiosityElement.unShotImportance = stochasticRadiosityElement.importance;
      Mcrad.monteCarloRadiosityAccumulateImportances(stochasticRadiosityElement);
    }
    else {
      stochasticRadiosityElement.importance = 0.0;
      stochasticRadiosityElement.sourceImportance = 0.0;
      stochasticRadiosityElement.unShotImportance = 0.0;
      stochasticRadiosityElement.traverseAllChildren((child) => Mcrad.monteCarloRadiosityPullImportances(child));
    }
  }

  public static monteCarloRadiosityUpdateViewImportance(scene: Scene, renderOptions: RendererConfiguration): void {
    process.stderr.write("Updating direct visibility ... \n");

    Potential.updateDirectVisibility(scene, renderOptions);

    StochasticRelaxation.activeState().sourceYmp = 0.0;
    StochasticRelaxation.activeState().unShotYmp = 0.0;
    StochasticRelaxation.activeState().totalYmp = 0.0;
    Mcrad.monteCarloRadiosityUpdateImportance(ElementHierarchyState.activeState().topCluster as Element);

    if (StochasticRelaxation.activeState().unShotYmp < StochasticRelaxation.activeState().sourceYmp) {
      process.stderr.write("Importance will be recomputed incrementally.\n");
      StochasticRelaxation.activeState().importanceUpdatedFromScratch = 0;
    }
    else {
      process.stderr.write("Importance will be recomputed from scratch.\n");
      StochasticRelaxation.activeState().importanceUpdatedFromScratch = 1;

      StochasticRelaxation.activeState().sourceYmp = 0.0;
      StochasticRelaxation.activeState().unShotYmp = 0.0;
      StochasticRelaxation.activeState().totalYmp = 0.0;
      Mcrad.monteCarloRadiosityReInitImportance(ElementHierarchyState.activeState().topCluster as Element);
    }

    if (scene.camera !== null) {
      scene.camera.changed = 0;
    }
    StochasticRelaxation.activeState().importanceTracedRays = 0;
    StochasticRelaxation.activeState().importanceUpdated = 1;
  }

  /**
Computes max_i (A_T/A_i): the ratio of the total area over the minimal patch
area in the scene, ignoring the 10% area occupied by the smallest patches
*/
  private static monteCarloRadiosityDetermineAreaFraction(
    scenePatches: ArrayList<Patch>,
    sceneGeometries: ArrayList<Geometry>
  ): number {
    const numberOfPatchIds = Patch.getNextId();

    if (sceneGeometries === null || sceneGeometries.size() === 0) {
      return 100;
    }

    const areas = new Array<number>(numberOfPatchIds).fill(0.0);
    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      const patch = scenePatches.get(i);
      areas[patch.id] = patch.area;
    }

    areas.sort((a, b) => a - b);

    let i = 0;
    let cumulative = 0.0;
    for (i = numberOfPatchIds - 1; i >= 0 && cumulative < Statistics.instance().radiance.totalArea * 0.1; i--) {
      cumulative += areas[i]!;
    }
    const areaFrac = (i >= 0 && areas[i]! > 0.0)
      ? Statistics.instance().radiance.totalArea / areas[i]!
      : Statistics.instance().reader.numberOfPatches;

    return areaFrac;
  }

  /**
Determines elementary ray power for the initial incremental iterations
*/
  private static monteCarloRadiosityDetermineInitialNrRays(
    scenePatches: ArrayList<Patch>,
    sceneGeometries: ArrayList<Geometry>
  ): void {
    const areaFrac = Mcrad.monteCarloRadiosityDetermineAreaFraction(scenePatches, sceneGeometries);
    StochasticRelaxation.activeState().initialNumberOfRays =
      globalThis.Math.trunc(StochasticRelaxation.activeState().rayUnitsPerIt * areaFrac);
  }

  /**
Really initialises: before the first iteration step
*/
  public static monteCarloRadiosityReInit(scene: Scene, renderOptions: RendererConfiguration): void {
    if (StochasticRelaxation.activeState().inited !== 0) {
      return;
    }

    process.stderr.write("Initialising Monte Carlo radiosity ...\n");

    Sample4d.setSequence4D(
      StochasticRelaxation.activeState().sequence ?? Sampler4DSequence.NIEDERREITER
    );

    StochasticRelaxation.activeState().inited = 1;
    StochasticRelaxation.activeState().cpuSeconds = 0.0;
    StochasticRelaxation.activeState().lastClock = Number(process.hrtime.bigint());
    StochasticRelaxation.activeState().currentIteration = 0;
    StochasticRelaxation.activeState().tracedRays = 0;
    StochasticRelaxation.activeState().prevTracedRays = 0;
    StochasticRelaxation.activeState().numberOfMisses = 0;
    StochasticRelaxation.activeState().importanceTracedRays = 0;
    StochasticRelaxation.activeState().prevImportanceTracedRays = 0;
    StochasticRelaxation.activeState().setSource = StochasticRelaxation.activeState().indirectOnly;
    StochasticRelaxation.activeState().tracedPaths = 0;
    StochasticRelaxation.activeState().controlRadiance.clear();

    StochasticRelaxation.activeState().unShotFlux.clear();
    StochasticRelaxation.activeState().unShotYmp = 0.0;
    StochasticRelaxation.activeState().totalFlux.clear();
    StochasticRelaxation.activeState().totalYmp = 0.0;
    StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.clear();

    const scenePatches = new ArrayList<Patch>();
    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      scenePatches.add(scene.patchList[i]!);
    }

    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      const patch = scene.patchList[i]!;
      Mcrad.monteCarloRadiosityInitPatch(patch);
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
        globalThis.Math.PI * patch.area *
        (McradP.topLevelStochasticRadiosityElement(patch).importance - McradP.topLevelStochasticRadiosityElement(patch).sourceImportance),
        McradP.getTopLevelPatchUnShotRad(patch)![0]!
      );
      StochasticRelaxation.activeState().unShotYmp += patch.area * globalThis.Math.abs(McradP.topLevelStochasticRadiosityElement(patch).unShotImportance);
      StochasticRelaxation.activeState().totalYmp += patch.area * McradP.topLevelStochasticRadiosityElement(patch).importance;
      StochasticRelaxation.activeState().sourceYmp += patch.area * McradP.topLevelStochasticRadiosityElement(patch).sourceImportance;
      Mcrad.monteCarloRadiosityPatchComputeNewColor(patch);
    }

    const sceneGeometries = new ArrayList<Geometry>();
    for (let i = 0; scene.geometryList !== null && i < scene.geometryList.length; i++) {
      sceneGeometries.add(scene.geometryList[i]!);
    }

    Mcrad.monteCarloRadiosityDetermineInitialNrRays(scenePatches, sceneGeometries);

    Hierarchy.elementHierarchyInit(scene.clusteredRootGeometry as Geometry);

    if (StochasticRelaxation.activeState().importanceDriven !== 0) {
      Mcrad.monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
      StochasticRelaxation.activeState().importanceUpdatedFromScratch = 1;
    }
  }

  public static monteCarloRadiosityPreStep(scene: Scene, renderOptions: RendererConfiguration): void {
    if (StochasticRelaxation.activeState().inited === 0) {
      Mcrad.monteCarloRadiosityReInit(scene, renderOptions);
    }
    if (StochasticRelaxation.activeState().importanceDriven !== 0 && scene.camera !== null && scene.camera.changed !== 0) {
      Mcrad.monteCarloRadiosityUpdateViewImportance(scene, renderOptions);
    }

    StochasticRelaxation.activeState().lastClock = Number(process.hrtime.bigint());
    StochasticRelaxation.activeState().currentIteration++;
  }

  /**
Undoes the effect of mainInitApplication() and all side-effects of Step()
*/
  public static monteCarloRadiosityTerminate(scenePatches: ArrayList<Patch>): void {
    Hierarchy.elementHierarchyTerminate(scenePatches);
    StochasticRelaxation.activeState().inited = 0;
  }

  private static monteCarloRadiosityDiffuseReflectanceAtPoint(patch: Patch, u: number, v: number): ColorRgb {
    const hit = new RayHit();
    const point = new Vector3D();
    patch.uniformPoint(u, v, point);
    hit.init(patch, point, patch.normal, patch.material);
    hit.setUv(u, v);
    const newFlags = hit.getFlags() | RayHitFlag.UV;
    hit.setFlags(newFlags);
    let result = new ColorRgb();
    result.clear();
    if (hit.getMaterial() !== null && hit.getMaterial()!.getBsdf() !== null) {
      result = hit.getMaterial()!.getBsdf()!.splitBsdfScatteredPower(hit.shadingContext(), BsdfComponent.BRDF_DIFFUSE_COMPONENT);
    }
    return result;
  }

  private static vertexReflectance(v: Vertex): ColorRgb {
    let count = 0;
    const rd = new ColorRgb();

    rd.clear();
    for (let i = 0; v.radianceData !== null && i < v.radianceData.length; i++) {
      const genericElement = v.radianceData[i]!;
      if (genericElement.className !== ElementTypes.ELEMENT_STOCHASTIC_RADIOSITY) {
        continue;
      }
      const element = genericElement as StochasticRadiosityElement;
      if (element.regularSubElements === null) {
        rd.add(rd, element.Rd);
        count++;
      }
    }

    if (count > 0) {
      rd.scaleInverse(count, rd);
    }

    return rd;
  }

  private static cachedLeaf: StochasticRadiosityElement | null = null;
  private static vrd: ColorRgb[] = [new ColorRgb(), new ColorRgb(), new ColorRgb(), new ColorRgb()];
  private static cachedRd = new ColorRgb();

  private static monteCarloRadiosityInterpolatedReflectanceAtPoint(
    leaf: StochasticRadiosityElement,
    u: number,
    v: number
  ): ColorRgb {
    if (leaf !== null) {
      if (leaf !== Mcrad.cachedLeaf) {
        for (let i = 0; i < leaf.numberOfVertices; i++) {
          Mcrad.vrd[i] = Mcrad.vertexReflectance(leaf.vertices[i] as Vertex);
        }
      }
      Mcrad.cachedLeaf = leaf;

      Mcrad.cachedRd.clear();
      switch (leaf.numberOfVertices) {
        case 3:
          Mcrad.cachedRd.interpolateBarycentric(Mcrad.vrd[0]!, Mcrad.vrd[1]!, Mcrad.vrd[2]!, u, v);
          break;
        case 4:
          Mcrad.cachedRd.interpolateBiLinear(Mcrad.vrd[0]!, Mcrad.vrd[1]!, Mcrad.vrd[2]!, Mcrad.vrd[3]!, u, v);
          break;
        default:
          VsdkLogger.fatal(-1, "monteCarloRadiosityInterpolatedReflectanceAtPoint", "Invalid nr of vertices %d", leaf.numberOfVertices);
          break;
      }
    }
    return Mcrad.cachedRd;
  }

  /**
Returns the radiance emitted from the patch at the point with parameters
(u,v) into the direction 'dir'
*/
  public static monteCarloRadiosityGetRadiance(
    patch: Patch,
    u: number,
    v: number,
    dir: Vector3D,
    renderOptions: RendererConfiguration
  ): ColorRgb {
    void dir;

    const trueRdAtPoint = Mcrad.monteCarloRadiosityDiffuseReflectanceAtPoint(patch, u, v);
    const uu = [u];
    const vv = [v];
    const leaf = StochasticRadiosityElement.stochasticRadiosityElementRegularLeafElementAtPoint(
      McradP.topLevelStochasticRadiosityElement(patch), uu, vv
    );
    const usedRdAtPoint = renderOptions.smoothShading
      ? Mcrad.monteCarloRadiosityInterpolatedReflectanceAtPoint(leaf, uu[0]!, vv[0]!)
      : leaf.Rd;
    const radianceAtPoint = StochasticRadiosityElement.stochasticRadiosityElementDisplayRadianceAtPoint(leaf, uu[0]!, vv[0]!, renderOptions);
    let sourceRad = new ColorRgb();
    sourceRad.clear();

    if (StochasticRelaxation.activeState().show !== WhatToShow.SHOW_INDIRECT_RADIANCE) {
      if (StochasticRelaxation.activeState().doNonDiffuseFirstShot === 0) {
        sourceRad = leaf.sourceRad;
      }
      if (StochasticRelaxation.activeState().indirectOnly !== 0 || StochasticRelaxation.activeState().doNonDiffuseFirstShot !== 0) {
        sourceRad.add(sourceRad, leaf.Ed);
      }
    }
    radianceAtPoint.subtract(radianceAtPoint, sourceRad);

    radianceAtPoint.scalarProduct(radianceAtPoint, trueRdAtPoint);
    radianceAtPoint.divide(radianceAtPoint, usedRdAtPoint);

    radianceAtPoint.add(radianceAtPoint, sourceRad);

    return radianceAtPoint;
  }

  /**
Returns scalar reflectance, for importance propagation
*/
  public static monteCarloRadiosityScalarReflectance(P: Patch): number {
    return StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(McradP.topLevelStochasticRadiosityElement(P));
  }
}
