import { OutputStream } from "../../../../java/io/OutputStream";
import { StringBuilder } from "../../../../java/lang/StringBuilder";
import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { RenderOptions } from "../../common/RenderOptions";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { Camera } from "../../scene/Camera";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "../../scene/RadianceMethodAlgorithm";
import { Scene } from "../../scene/Scene";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Element } from "../../skin/Element";
import { Patch } from "../../skin/Patch";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { Coefficientsmcrad } from "./Coefficientsmcrad";
import { ElementHierarchyState } from "./ElementHierarchyState";
import { HierarchyClusteringMode } from "./HierarchyClusteringMode";
import { Mcrad } from "./Mcrad";
import { Nondiff } from "./Nondiff";
import { StochasticJacobi } from "./StochasticJacobi";
import { StochasticRadiosityBasisState } from "./StochasticRadiosityBasisState";
import { StochasticRadiosityElement } from "./StochasticRadiosityElement";
import { StochasticRaytracingApproximation } from "./StochasticRaytracingApproximation";
import { StochasticRaytracingMethod } from "./StochasticRaytracingMethod";
import { StochasticRelaxation } from "./StochasticRelaxation";

const util = require("node:util");

/**
Stochastic Relaxation Radiosity (currently only stochastic Jacobi)
*/
export class StochasticJacobiRadianceMethod extends RadianceMethod {
  private static readonly STRING_LENGTH = 2000;
  private readonly stochasticRelaxationState: StochasticRelaxation;
  private readonly elementHierarchyState: ElementHierarchyState;
  private readonly stochasticRadiosityBasisState: StochasticRadiosityBasisState;

  private static appendStochasticStatsText(
    buffer: StringBuilder,
    offset: number[],
    format: string,
    ...args: unknown[]
  ): void {
    if (offset[0] >= StochasticJacobiRadianceMethod.STRING_LENGTH - 1) {
      return;
    }

    let text: string;
    try {
      text = util.format(format, ...args);
    }
    catch (_e) {
      text = format;
    }

    const available = StochasticJacobiRadianceMethod.STRING_LENGTH - offset[0];
    if (available <= 0) {
      return;
    }
    if (text.length >= available) {
      buffer.append(text, available - 1);
      offset[0] = StochasticJacobiRadianceMethod.STRING_LENGTH - 1;
    }
    else {
      buffer.append(text);
      offset[0] += text.length;
    }
  }

  public constructor(
    inStochasticRelaxationState: StochasticRelaxation,
    inElementHierarchyState: ElementHierarchyState,
    inStochasticRadiosityBasisState: StochasticRadiosityBasisState
  ) {
    super();
    this.stochasticRelaxationState = inStochasticRelaxationState;
    this.elementHierarchyState = inElementHierarchyState;
    this.stochasticRadiosityBasisState = inStochasticRadiosityBasisState;

    StochasticRelaxation.setActiveState(this.stochasticRelaxationState);
    ElementHierarchyState.setActiveState(this.elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(this.stochasticRadiosityBasisState);
    Mcrad.monteCarloRadiosityDefaults();
    this.className = RadianceMethodAlgorithm.STOCHASTIC_JACOBI;
  }

  public override getRadianceMethodName(): string {
    return "Stochastic Jacobi";
  }

  public override parseOptions(argc: number[], argv: string[]): void {
    void argc;
    void argv;
  }

  private static toArrayList(scenePatches: Patch[] | null): ArrayList<Patch> {
    const out = new ArrayList<Patch>();
    for (let i = 0; scenePatches !== null && i < scenePatches.length; i++) {
      out.add(scenePatches[i]);
    }
    return out;
  }

  public override terminate(scenePatches: Patch[]): void {
    StochasticRelaxation.setActiveState(this.stochasticRelaxationState);
    ElementHierarchyState.setActiveState(this.elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(this.stochasticRadiosityBasisState);
    Mcrad.monteCarloRadiosityTerminate(StochasticJacobiRadianceMethod.toArrayList(scenePatches));
  }

  public override getRadiance(
    camera: Camera,
    patch: Patch,
    u: number,
    v: number,
    dir: Vector3D,
    renderOptions: RenderOptions
  ): ColorRgb {
    StochasticRelaxation.setActiveState(this.stochasticRelaxationState);
    ElementHierarchyState.setActiveState(this.elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(this.stochasticRadiosityBasisState);
    if (camera === null) {
      // camera is intentionally unused in C++ implementation.
    }
    return Mcrad.monteCarloRadiosityGetRadiance(patch, u, v, dir, renderOptions);
  }

  public override createPatchData(patch: Patch): Element {
    return Mcrad.monteCarloRadiosityCreatePatchData(patch);
  }

  public override destroyPatchData(patch: Patch): void {
    Mcrad.monteCarloRadiosityDestroyPatchData(patch);
  }

  public override writeVRML(
    camera: Camera,
    outputStream: OutputStream,
    renderOptions: RenderOptions
  ): void {
    if (camera === null || outputStream === null || renderOptions === null) {
      // Not implemented in C++ version either.
    }
  }

  public override initialize(scene: Scene, toneMapOptions: ToneMappingContext): void {
    StochasticRelaxation.setActiveState(this.stochasticRelaxationState);
    ElementHierarchyState.setActiveState(this.elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(this.stochasticRadiosityBasisState);
    if (scene === null) {
      // scene is intentionally unused in C++ implementation.
    }
    StochasticRelaxation.activeState().toneMapOptions = toneMapOptions;
    if (StochasticRelaxation.activeState().toneMapOptions === null) {
      VsdkLogger.fatal(-1, "StochasticJacobiRadianceMethod::initialize", "Tone mapping context not provided");
    }
    StochasticRelaxation.activeState().method = StochasticRaytracingMethod.STOCHASTIC_RELAXATION_RADIOSITY_METHOD;
    Mcrad.monteCarloRadiosityInit();
  }

  public override getStats(): string {
    StochasticRelaxation.setActiveState(this.stochasticRelaxationState);
    ElementHierarchyState.setActiveState(this.elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(this.stochasticRadiosityBasisState);

    const stats = new StringBuilder();
    const statsOffset = [0];

    StochasticJacobiRadianceMethod.appendStochasticStatsText(stats, statsOffset, "Stochastic Relaxation Radiosity\nStatistics\n\n");
    StochasticJacobiRadianceMethod.appendStochasticStatsText(stats, statsOffset, "Iteration nr: %d\n", StochasticRelaxation.activeState().currentIteration);
    StochasticJacobiRadianceMethod.appendStochasticStatsText(stats, statsOffset, "CPU time: %g secs\n", StochasticRelaxation.activeState().cpuSeconds);
    StochasticJacobiRadianceMethod.appendStochasticStatsText(
      stats,
      statsOffset,
      "%d elements (%d clusters, %d surfaces)\n",
      ElementHierarchyState.activeState().nr_elements,
      ElementHierarchyState.activeState().nr_clusters,
      ElementHierarchyState.activeState().nr_elements - ElementHierarchyState.activeState().nr_clusters
    );
    StochasticJacobiRadianceMethod.appendStochasticStatsText(stats, statsOffset, "Radiance rays: %d\n", StochasticRelaxation.activeState().tracedRays);
    StochasticJacobiRadianceMethod.appendStochasticStatsText(stats, statsOffset, "Importance rays: %d\n", StochasticRelaxation.activeState().importanceTracedRays);

    return stats.toString();
  }

  /**
Randomly returns floor(x) or ceil(x) so that the expected value is equal to x
*/
  private static stochasticRelaxationRadiosityRandomRound(x: number): number {
    let l = globalThis.Math.floor(x);
    if (globalThis.Math.random() < (x - l)) {
      l++;
    }
    return l;
  }

  private static stochasticRelaxationRadiosityRecomputeDisplayColors(scenePatches: ArrayList<Patch>): void {
    const topElement = ElementHierarchyState.activeState().topCluster as StochasticRadiosityElement;
    if (topElement !== null) {
      topElement.traverseClusterLeafElements(StochasticRadiosityElement.stochasticRadiosityElementComputeNewVertexColors);
      topElement.traverseClusterLeafElements(StochasticRadiosityElement.stochasticRadiosityElementAdjustTVertexColors);
    }
    else {
      for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
        Mcrad.monteCarloRadiosityPatchComputeNewColor(scenePatches.get(i));
      }
    }
  }

  /**
Computes quality factor on given leaf element.
*/
  private static stochasticRelaxationRadiosityQualityFactor(elem: StochasticRadiosityElement, w: number): number {
    if (StochasticRelaxation.activeState().importanceDriven !== 0) {
      return w * elem.importance;
    }
    return w / StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(elem);
  }

  private static stochasticRelaxationRadiosityElementUnShotRadiance(elem: StochasticRadiosityElement): ColorRgb[] {
    return elem.unShotRadiance!;
  }

  private static stochasticRelaxationRadiosityElementIncrementRadiance(elem: StochasticRadiosityElement, w: number): void {
    if (StochasticRelaxation.activeState().discardIncremental !== 0) {
      elem.quality = 0.0;
      if (!StochasticJacobiRadianceMethod.repeatedDiscardIncrementalWarning) {
        VsdkLogger.warning(
          "stochasticRelaxationRadiosityElementIncrementRadiance",
          "Solution of incremental Jacobi steps receives zero quality"
        );
      }
      StochasticJacobiRadianceMethod.repeatedDiscardIncrementalWarning = true;
    }
    else {
      elem.quality = StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityQualityFactor(elem, w);
    }

    Coefficientsmcrad.stochasticRadiosityAddCoefficients(elem.radiance, elem.receivedRadiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityCopyCoefficients(elem.unShotRadiance, elem.receivedRadiance, elem.basis);
    if (StochasticRelaxation.activeState().setSource !== 0) {
      elem.radiance![0].set(elem.receivedRadiance![0].r, elem.receivedRadiance![0].g, elem.receivedRadiance![0].b);
      elem.sourceRad.set(elem.receivedRadiance![0].r, elem.receivedRadiance![0].g, elem.receivedRadiance![0].b);
    }
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
  }

  private static stochasticRelaxationRadiosityPrintIncrementalRadianceStats(): void {
    process.stderr.write(
      util.format(
        "%g secs., radiance rays = %d (%d not to background), un-shot flux = ",
        StochasticRelaxation.activeState().cpuSeconds,
        StochasticRelaxation.activeState().tracedRays,
        StochasticRelaxation.activeState().tracedRays - StochasticRelaxation.activeState().numberOfMisses
      )
    );
    StochasticRelaxation.activeState().unShotFlux.print(process.stderr);
    process.stderr.write(", total flux = ");
    StochasticRelaxation.activeState().totalFlux.print(process.stderr);
    process.stderr.write(", indirect importance weighted un-shot flux = ");
    StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.print(process.stderr);
    process.stderr.write("\n");
  }

  private static stochasticRelaxationRadiosityDoIncrementalRadianceIterations(
    scene: Scene,
    radianceMethod: RadianceMethod,
    renderOptions: RenderOptions
  ): void {
    let refUnShot: number;
    let stepNumber = 0;

    const weightedSampling = StochasticRelaxation.activeState().weightedSampling;
    const importanceDriven = StochasticRelaxation.activeState().importanceDriven;
    if (StochasticRelaxation.activeState().incrementalUsesImportance === 0) {
      StochasticRelaxation.activeState().importanceDriven = 0;
    }
    StochasticRelaxation.activeState().weightedSampling = 0;

    StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
    refUnShot = StochasticRelaxation.activeState().unShotFlux.sumAbsComponents();
    if (StochasticRelaxation.activeState().incrementalUsesImportance !== 0) {
      refUnShot = StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.sumAbsComponents();
    }
    while (true) {
      let unShotFraction: number;
      let numberOfRays: number;
      unShotFraction = StochasticRelaxation.activeState().unShotFlux.sumAbsComponents() / refUnShot;
      if (StochasticRelaxation.activeState().incrementalUsesImportance !== 0) {
        unShotFraction = StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.sumAbsComponents() / refUnShot;
      }
      if (unShotFraction < 0.01) {
        break;
      }
      const approx = (StochasticRelaxation.activeState().approximationOrderType
        ?? StochasticRaytracingApproximation.CONSTANT) as number;
      numberOfRays = StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityRandomRound(
        unShotFraction * StochasticRelaxation.activeState().initialNumberOfRays
        * StochasticRadiosityBasisState.activeState().approxDesc[approx].basis_size
      );

      stepNumber++;
      process.stderr.write(
        util.format(
          "Incremental radiance propagation step %d: %.3f%% un-shot power left.\n",
          stepNumber,
          100.0 * unShotFraction
        )
      );

      StochasticJacobi.doStochasticJacobiIteration(
        scene.voxelGrid as VoxelGrid,
        numberOfRays,
        StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementUnShotRadiance,
        null,
        StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementIncrementRadiance,
        StochasticJacobiRadianceMethod.toArrayList(scene.patchList),
        renderOptions
      );

      StochasticRelaxation.activeState().setSource = 0;

      Mcrad.monteCarloRadiosityUpdateCpuSecs();
      StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
      if (unShotFraction > 0.3) {
        StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityRecomputeDisplayColors(
          StochasticJacobiRadianceMethod.toArrayList(scene.patchList)
        );
      }
    }

    StochasticRelaxation.activeState().importanceDriven = importanceDriven;
    StochasticRelaxation.activeState().weightedSampling = weightedSampling;

    if (radianceMethod === null) {
      // radianceMethod is intentionally unused in C++ implementation.
    }
  }

  private static stochasticRelaxationRadiosityElementUnShotImportance(elem: StochasticRadiosityElement): number {
    return elem.unShotImportance;
  }

  private static stochasticRelaxationRadiosityElementIncrementImportance(elem: StochasticRadiosityElement, w: number): void {
    if (w < -1) {
      // Keep C++ signature.
    }
    elem.importance += elem.receivedImportance;
    elem.unShotImportance = elem.receivedImportance;
    elem.receivedImportance = 0.0;
  }

  private static stochasticRelaxationRadiosityPrintIncrementalImportanceStats(): void {
    process.stderr.write(
      util.format(
        "%g secs., importance rays = %d, un-shot importance = %g, total importance = %g, total area = %g\n",
        StochasticRelaxation.activeState().cpuSeconds,
        StochasticRelaxation.activeState().importanceTracedRays,
        StochasticRelaxation.activeState().unShotYmp,
        StochasticRelaxation.activeState().totalYmp,
        Statistics.instance().radiance.totalArea
      )
    );
  }

  private static stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
    sceneWorldVoxelGrid: VoxelGrid,
    scenePatches: ArrayList<Patch>,
    renderOptions: RenderOptions
  ): void {
    let stepNumber = 0;
    const radianceDriven = StochasticRelaxation.activeState().radianceDriven;
    const doHMeshing = ElementHierarchyState.activeState().do_h_meshing;
    const clustering = ElementHierarchyState.activeState().clustering;
    const weightedSampling = StochasticRelaxation.activeState().weightedSampling;

    if (StochasticRelaxation.activeState().sourceYmp < Numeric.EPSILON) {
      process.stderr.write("No source importance!!\n");
      return;
    }

    StochasticRelaxation.activeState().radianceDriven = 0;
    ElementHierarchyState.activeState().do_h_meshing = 0;
    ElementHierarchyState.activeState().clustering = HierarchyClusteringMode.NO_CLUSTERING;
    StochasticRelaxation.activeState().weightedSampling = 0;

    StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
    while (true) {
      const unShotFraction = StochasticRelaxation.activeState().unShotYmp / StochasticRelaxation.activeState().sourceYmp;
      const numberOfRays = StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityRandomRound(
        unShotFraction * StochasticRelaxation.activeState().initialNumberOfRays
      );
      if (unShotFraction < 0.01) {
        break;
      }

      stepNumber++;
      process.stderr.write(
        util.format(
          "Incremental importance propagation step %d: %.3f%% un-shot importance left.\n",
          stepNumber,
          100.0 * unShotFraction
        )
      );

      StochasticJacobi.doStochasticJacobiIteration(
        sceneWorldVoxelGrid,
        numberOfRays,
        null,
        StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementUnShotImportance,
        StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementIncrementImportance,
        scenePatches,
        renderOptions
      );

      Mcrad.monteCarloRadiosityUpdateCpuSecs();
      StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityPrintIncrementalImportanceStats();
    }

    StochasticRelaxation.activeState().radianceDriven = radianceDriven;
    ElementHierarchyState.activeState().do_h_meshing = doHMeshing;
    ElementHierarchyState.activeState().clustering = clustering;
    StochasticRelaxation.activeState().weightedSampling = weightedSampling;
  }

  private static stochasticRelaxationRadiosityElementRadiance(elem: StochasticRadiosityElement): ColorRgb[] {
    return elem.radiance!;
  }

  private static stochasticRelaxationRadiosityElementUpdateRadiance(elem: StochasticRadiosityElement, w: number): void {
    let k = StochasticRelaxation.activeState().prevTracedRays /
      (StochasticRelaxation.activeState().tracedRays > 0 ? StochasticRelaxation.activeState().tracedRays : 1);

    if (StochasticRelaxation.activeState().naiveMerging === 0) {
      const quality = StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityQualityFactor(elem, w);
      if (elem.quality < Numeric.EPSILON) {
        k = 0.0;
      }
      else if (quality < Numeric.EPSILON) {
        k = 1.0;
      }
      else if (elem.quality + quality > Numeric.EPSILON) {
        k = elem.quality / (elem.quality + quality);
      }
      else {
        k = 0.0;
      }
      elem.quality += quality;
    }

    elem.radiance![0].subtract(elem.radiance![0], elem.sourceRad);

    Coefficientsmcrad.stochasticRadiosityScaleCoefficients(k, elem.radiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityScaleCoefficients((1.0 - k), elem.receivedRadiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityAddCoefficients(elem.radiance, elem.receivedRadiance, elem.basis);

    elem.radiance![0].add(elem.radiance![0], elem.sourceRad);

    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
  }

  private static stochasticRelaxationRadiosityPrintRegularStats(): void {
    process.stderr.write(
      util.format(
        "%g secs., radiance rays = %d (%d not to background), un-shot flux = ",
        StochasticRelaxation.activeState().cpuSeconds,
        StochasticRelaxation.activeState().tracedRays,
        StochasticRelaxation.activeState().tracedRays - StochasticRelaxation.activeState().numberOfMisses
      )
    );
    process.stderr.write(", total flux = ");
    StochasticRelaxation.activeState().totalFlux.print(process.stderr);
    if (StochasticRelaxation.activeState().importanceDriven !== 0) {
      process.stderr.write(
        util.format(
          "\ntotal importance rays = %d, total importance = %g, total area = %g",
          StochasticRelaxation.activeState().importanceTracedRays,
          StochasticRelaxation.activeState().totalYmp,
          Statistics.instance().radiance.totalArea
        )
      );
    }
    process.stderr.write("\n");
  }

  private static stochasticRelaxationRadiosityDoRegularRadianceIteration(
    sceneWorldVoxelGrid: VoxelGrid,
    scenePatches: ArrayList<Patch>,
    renderOptions: RenderOptions
  ): void {
    process.stderr.write(
      util.format("Regular radiance iteration %d:\n", StochasticRelaxation.activeState().currentIteration)
    );
    StochasticJacobi.doStochasticJacobiIteration(
      sceneWorldVoxelGrid,
      StochasticRelaxation.activeState().raysPerIteration,
      StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementRadiance,
      null,
      StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementUpdateRadiance,
      scenePatches,
      renderOptions
    );

    Mcrad.monteCarloRadiosityUpdateCpuSecs();
    StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityPrintRegularStats();
  }

  private static stochasticRelaxationRadiosityElementImportance(elem: StochasticRadiosityElement): number {
    return elem.importance;
  }

  private static stochasticRelaxationRadiosityElementUpdateImportance(elem: StochasticRadiosityElement, w: number): void {
    if (w < -1) {
      // Keep C++ signature.
    }
    const k = StochasticRelaxation.activeState().prevImportanceTracedRays /
      StochasticRelaxation.activeState().importanceTracedRays;

    elem.importance =
      k * (elem.importance - elem.sourceImportance) + (1.0 - k) * elem.receivedImportance + elem.sourceImportance;
    elem.unShotImportance = 0.0;
    elem.receivedImportance = 0.0;
  }

  private static stochasticRelaxationRadiosityDoRegularImportanceIteration(
    sceneWorldVoxelGrid: VoxelGrid,
    scenePatches: ArrayList<Patch>,
    renderOptions: RenderOptions
  ): void {
    let numberOfRays: number;
    const doHierarchicMeshing = ElementHierarchyState.activeState().do_h_meshing;
    const clustering = ElementHierarchyState.activeState().clustering;
    const weightedSampling = StochasticRelaxation.activeState().weightedSampling;
    ElementHierarchyState.activeState().do_h_meshing = 0;
    ElementHierarchyState.activeState().clustering = HierarchyClusteringMode.NO_CLUSTERING;
    StochasticRelaxation.activeState().weightedSampling = 0;

    numberOfRays = StochasticRelaxation.activeState().importanceRaysPerIteration;
    process.stderr.write(
      util.format("Regular importance iteration %d:\n", StochasticRelaxation.activeState().currentIteration)
    );

    StochasticJacobi.doStochasticJacobiIteration(
      sceneWorldVoxelGrid,
      numberOfRays,
      null,
      StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementImportance,
      StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementUpdateImportance,
      scenePatches,
      renderOptions
    );

    Mcrad.monteCarloRadiosityUpdateCpuSecs();
    StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityPrintRegularStats();

    ElementHierarchyState.activeState().do_h_meshing = doHierarchicMeshing;
    ElementHierarchyState.activeState().clustering = clustering;
    StochasticRelaxation.activeState().weightedSampling = weightedSampling;
  }

  /**
Resets to zero all kind of things that should be reset to zero after a first
iteration.
*/
  private static stochasticRelaxationRadiosityElementDiscardIncremental(element: Element): void {
    const stochasticRadiosityElement = element as StochasticRadiosityElement;

    if (stochasticRadiosityElement === null) {
      return;
    }

    stochasticRadiosityElement.quality = 0.0;
    stochasticRadiosityElement.traverseAllChildren(StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementDiscardIncremental);
  }

  private static stochasticRelaxationRadiosityDiscardIncremental(): void {
    StochasticRelaxation.activeState().tracedRays = 0;
    StochasticRelaxation.activeState().prevTracedRays = 0;

    StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityElementDiscardIncremental(
      ElementHierarchyState.activeState().topCluster as Element
    );
  }

  public override doStep(scene: Scene, renderOptions: RenderOptions): boolean {
    StochasticRelaxation.setActiveState(this.stochasticRelaxationState);
    ElementHierarchyState.setActiveState(this.elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(this.stochasticRadiosityBasisState);
    Mcrad.monteCarloRadiosityPreStep(scene, renderOptions);

    const scenePatches = StochasticJacobiRadianceMethod.toArrayList(scene.patchList);

    if (StochasticRelaxation.activeState().currentIteration === 1) {
      if (StochasticRelaxation.activeState().doNonDiffuseFirstShot !== 0) {
        Nondiff.doNonDiffuseFirstShot(scene, this, renderOptions);
      }
      const initialNrOfRays = StochasticRelaxation.activeState().tracedRays;

      if (StochasticRelaxation.activeState().importanceDriven !== 0) {
        if (StochasticRelaxation.activeState().incrementalUsesImportance === 0) {
          VsdkLogger.warning(null, "Importance is only used from the second iteration on ...");
        }
        else if (StochasticRelaxation.activeState().importanceUpdated !== 0) {
          StochasticRelaxation.activeState().importanceUpdated = 0;

          StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
            scene.voxelGrid as VoxelGrid,
            scenePatches,
            renderOptions
          );
          if (StochasticRelaxation.activeState().importanceUpdatedFromScratch !== 0) {
            StochasticRelaxation.activeState().importanceRaysPerIteration =
              StochasticRelaxation.activeState().importanceTracedRays;
          }
        }
      }
      StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityDoIncrementalRadianceIterations(
        scene,
        this,
        renderOptions
      );

      StochasticRelaxation.activeState().raysPerIteration =
        StochasticRelaxation.activeState().tracedRays - initialNrOfRays;

      if (StochasticRelaxation.activeState().discardIncremental !== 0) {
        StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityDiscardIncremental();
      }
    }
    else {
      if (StochasticRelaxation.activeState().importanceDriven !== 0) {
        if (StochasticRelaxation.activeState().importanceUpdated !== 0) {
          StochasticRelaxation.activeState().importanceUpdated = 0;

          StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
            scene.voxelGrid as VoxelGrid,
            scenePatches,
            renderOptions
          );
          if (StochasticRelaxation.activeState().importanceUpdatedFromScratch !== 0) {
            StochasticRelaxation.activeState().importanceRaysPerIteration =
              StochasticRelaxation.activeState().importanceTracedRays;
          }
        }
        else {
          StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityDoRegularImportanceIteration(
            scene.voxelGrid as VoxelGrid,
            scenePatches,
            renderOptions
          );
        }
      }
      StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityDoRegularRadianceIteration(
        scene.voxelGrid as VoxelGrid,
        scenePatches,
        renderOptions
      );
    }

    StochasticJacobiRadianceMethod.stochasticRelaxationRadiosityRecomputeDisplayColors(scenePatches);

    process.stderr.write(`${this.getStats()}\n`);

    return false;
  }

  private static repeatedDiscardIncrementalWarning = false;
}
