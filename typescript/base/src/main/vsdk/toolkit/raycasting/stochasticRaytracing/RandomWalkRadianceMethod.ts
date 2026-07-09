import { OutputStream } from "../../../../java/io/OutputStream";
import { String as JavaString } from "../../../../java/lang/String";
import { StringBuilder } from "../../../../java/lang/StringBuilder";
import { System } from "../../../../java/lang/System";
import { ArrayList } from "../../../../java/util/ArrayList";
import { Cie } from "../../common/color/Cie";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { RendererConfiguration } from "../../material/RendererConfiguration";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Statistics } from "../../common/statistics/Statistics";
import { Camera } from "../../scene/Camera";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "../../scene/RadianceMethodAlgorithm";
import { Scene } from "../../scene/Scene";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { Element } from "../../environment/geometry/elements/Element";
import { Patch } from "../../environment/geometry/elements/Patch";
import { ToneMappingContext } from "../../tonemap/ToneMappingContext";
import { Coefficientsmcrad } from "./Coefficientsmcrad";
import { ElementHierarchyState } from "./ElementHierarchyState";
import { GalerkinBasis } from "./GalerkinBasis";
import { Mcrad } from "./Mcrad";
import { McradP } from "./McradP";
import { Path } from "./Path";
import { RandomWalkEstimatorKind } from "./RandomWalkEstimatorKind";
import { RandomWalkEstimatorType } from "./RandomWalkEstimatorType";
import { StochasticRadiosityBasisState } from "./StochasticRadiosityBasisState";
import { StochasticRadiosityElement } from "./StochasticRadiosityElement";
import { StochasticRaytracingPathNode } from "./StochasticRaytracingPathNode";
import { StochasticRelaxation } from "./StochasticRelaxation";
import { StochasticRaytracingApproximation } from "./StochasticRaytracingApproximation";
import { StochasticRaytracingMethod } from "./StochasticRaytracingMethod";
import { StochasticJacobi } from "./StochasticJacobi";
import { Tracepath } from "./Tracepath";

export class RandomWalkRadianceMethod extends RadianceMethod {
  private static readonly STRING_LENGTH = 2000;
  private readonly stochasticRelaxationState: StochasticRelaxation;
  private readonly elementHierarchyState: ElementHierarchyState;
  private readonly stochasticRadiosityBasisState: StochasticRadiosityBasisState;
  private static readonly selfEmittedRadiance = new Array<ColorRgb>(GalerkinBasis.MAX_BASIS_SIZE);

  static {
    for (let i = 0; i < RandomWalkRadianceMethod.selfEmittedRadiance.length; i++) {
      RandomWalkRadianceMethod.selfEmittedRadiance[i] = new ColorRgb();
    }
  }

  private static appendRandomWalkStatsText(
    buffer: StringBuilder,
    offset: number[],
    format: string,
    ...args: unknown[]
  ): void {
    if (offset[0]! >= RandomWalkRadianceMethod.STRING_LENGTH - 1) {
      return;
    }

    let text: string;
    try {
      text = JavaString.vformat(format, args);
    }
    catch (_e) {
      text = format;
    }

    const available = RandomWalkRadianceMethod.STRING_LENGTH - offset[0]!;
    if (available <= 0) {
      return;
    }

    if (text.length >= available) {
      buffer.append(text, available - 1);
      offset[0] = RandomWalkRadianceMethod.STRING_LENGTH - 1;
    }
    else {
      buffer.append(text);
      offset[0]! += text.length;
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
    this.className = RadianceMethodAlgorithm.RANDOM_WALK;
  }

  public override getRadianceMethodName(): string {
    return "Random walk";
  }

  public override parseOptions(argc: number[], argv: string[]): void {
    void argc;
    void argv;
  }

  public override getRadiance(
    camera: Camera,
    patch: Patch,
    u: number,
    v: number,
    dir: Vector3D,
    renderOptions: RendererConfiguration
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
    renderOptions: RendererConfiguration
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
      VsdkLogger.fatal(-1, "RandomWalkRadianceMethod::initialize", "Tone mapping context not provided");
    }
    StochasticRelaxation.activeState().method = StochasticRaytracingMethod.RANDOM_WALK_RADIOSITY_METHOD;
    Mcrad.monteCarloRadiosityInit();
  }

  private static toArrayList(scenePatches: Patch[] | null): ArrayList<Patch> {
    const out = new ArrayList<Patch>();
    for (let i = 0; scenePatches !== null && i < scenePatches.length; i++) {
      out.add(scenePatches[i]!);
    }
    return out;
  }

  private static randomWalkRadiosityPrintStats(): void {
    System.err.printf(
      "%g secs., total radiance rays = %d",
      StochasticRelaxation.activeState().cpuSeconds,
      StochasticRelaxation.activeState().tracedRays
    );
    System.err.printf(", total flux = ");
    StochasticRelaxation.activeState().totalFlux.print(System.err);
    if (StochasticRelaxation.activeState().importanceDriven !== 0) {
      System.err.printf(
        "\ntotal importance rays = %d, total importance = %g, total area = %g",
        StochasticRelaxation.activeState().importanceTracedRays,
        StochasticRelaxation.activeState().totalYmp,
        Statistics.instance().radiance.totalArea
      );
    }
    System.err.printf("\n");
  }

  /**
Used as un-normalised stochasticJacobiProbability for mimicking global lines
*/
  private static randomWalkRadiosityPatchArea(patch: Patch): number {
    return patch.area;
  }

  /**
stochasticJacobiProbability proportional to power to be propagated
*/
  private static randomWalkRadiosityScalarSourcePower(patch: Patch): number {
    const radiance = McradP.topLevelStochasticRadiosityElement(patch).sourceRad;
    return patch.area * radiance.sumAbsComponents();
  }

  /**
Returns a double instead of a float in order to make it useful as
a survival stochasticJacobiProbability function
*/
  private static randomWalkRadiosityScalarReflectance(patch: Patch): number {
    return Mcrad.monteCarloRadiosityScalarReflectance(patch);
  }

  private static randomWalkRadiosityGetSelfEmittedRadiance(elem: StochasticRadiosityElement): ColorRgb[] {
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(RandomWalkRadianceMethod.selfEmittedRadiance, elem.basis);
    const ed = McradP.topLevelStochasticRadiosityElement(elem.patch as Patch).Ed;
    RandomWalkRadianceMethod.selfEmittedRadiance[0]!.set(ed.r, ed.g, ed.b);
    return RandomWalkRadianceMethod.selfEmittedRadiance;
  }

  /**
Subtracts (1 - rho) * control radiosity from the source radiosity of each patch
*/
  private static randomWalkRadiosityReduceSource(scenePatches: ArrayList<Patch>): void {
    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      const patch = scenePatches.get(i);
      const newSourceRadiance = new ColorRgb();
      const rho = McradP.topLevelStochasticRadiosityElement(patch).Rd;

      newSourceRadiance.setMonochrome(1.0);
      newSourceRadiance.subtract(newSourceRadiance, rho);
      newSourceRadiance.selfScalarProduct(StochasticRelaxation.activeState().controlRadiance);
      newSourceRadiance.subtract(McradP.topLevelStochasticRadiosityElement(patch).sourceRad, newSourceRadiance);
      McradP.topLevelStochasticRadiosityElement(patch).sourceRad.set(
        newSourceRadiance.r,
        newSourceRadiance.g,
        newSourceRadiance.b
      );
    }
  }

  private static randomWalkRadiosityScoreWeight(path: Path, nodeIndex: number): number {
    let w = 0.0;
    const t = path.numberOfNodes - ((StochasticRelaxation.activeState().randomWalkNumLast > 0)
      ? StochasticRelaxation.activeState().randomWalkNumLast : 1);

    switch (StochasticRelaxation.activeState().randomWalkEstimatorKind) {
      case RandomWalkEstimatorKind.RW_COLLISION:
        w = 1.0;
        break;
      case RandomWalkEstimatorKind.RW_ABSORPTION:
        if (nodeIndex === path.numberOfNodes - 1) {
          w = 1.0 / (1.0 - path.nodes![nodeIndex]!.probability);
        }
        break;
      case RandomWalkEstimatorKind.RW_SURVIVAL:
        if (nodeIndex < path.numberOfNodes - 1) {
          w = 1.0 / path.nodes![nodeIndex]!.probability;
        }
        break;
      case RandomWalkEstimatorKind.RW_LAST_BUT_NTH:
        if (nodeIndex === t - 1) {
          const lastNodeIndex = path.numberOfNodes - 1;
          w = 1.0 / (1.0 - path.nodes![lastNodeIndex]!.probability);
          for (let n = lastNodeIndex - 1; n >= nodeIndex; n--) {
            w /= path.nodes![n]!.probability;
          }
        }
        break;
      case RandomWalkEstimatorKind.RW_N_LAST:
        if (nodeIndex === t) {
          w = 1.0 / (1.0 - path.nodes![path.numberOfNodes - 1]!.probability);
        }
        else if (nodeIndex > t) {
          w = 1.0;
        }
        break;
      default:
        VsdkLogger.fatal(
          -1,
          "randomWalkRadiosityScoreWeight",
          "Unknown random walk estimator kind %d",
          StochasticRelaxation.activeState().randomWalkEstimatorKind as number
        );
        break;
    }
    return w;
  }

  private static randomWalkRadiosityShootingScore(
    path: Path,
    numberOfPaths: number,
    birthProbability: (patch: Patch) => number
  ): void {
    if (birthProbability === null) {
      // Keep C++ signature.
    }
    const accumPow = new ColorRgb();
    const firstNode = path.nodes![0]!;

    accumPow.scaledCopy(
      firstNode.patch!.area / firstNode.probability,
      McradP.topLevelStochasticRadiosityElement(firstNode.patch as Patch).sourceRad
    );
    for (let n = 1; n < path.numberOfNodes; n++) {
      const node = path.nodes![n]!;
      const uin = [0.0];
      const vin = [0.0];
      const uOut = [0.0];
      const vOut = [0.0];
      let r = 1.0;
      const patch = node.patch as Patch;
      const Rd = McradP.topLevelStochasticRadiosityElement(patch).Rd;
      accumPow.scalarProduct(accumPow, Rd);

      patch.uniformUv(node.inPoint, uin, vin);
      if (StochasticRelaxation.activeState().continuousRandomWalk === 0) {
        r = 0.0;
        if (n < path.numberOfNodes - 1) {
          patch.uniformUv(node.outpoint, uOut, vOut);
        }
      }

      const w = RandomWalkRadianceMethod.randomWalkRadiosityScoreWeight(path, n);
      const basis = McradP.getTopLevelPatchBasis(patch) as GalerkinBasis;
      for (let i = 0; i < basis.size; i++) {
          const dual = basis.dualFunction![i]!(uin[0]!, vin[0]!) / patch.area;
          McradP.getTopLevelPatchReceivedRad(patch)![i]!.addScaled(
            McradP.getTopLevelPatchReceivedRad(patch)![i]!,
            w * dual / numberOfPaths,
            accumPow
          );

        if (StochasticRelaxation.activeState().continuousRandomWalk === 0) {
          const basf = basis.function![i]!(uOut[0]!, vOut[0]!);
          r += dual * patch.area * basf;
        }
      }

      accumPow.scale(r / node.probability);
    }
  }

  private static randomWalkRadiosityShootingUpdate(patch: Patch, w: number): void {
    const oldQuality = McradP.topLevelStochasticRadiosityElement(patch).quality;
    McradP.topLevelStochasticRadiosityElement(patch).quality += w;
    if (McradP.topLevelStochasticRadiosityElement(patch).quality < Numeric.EPSILON) {
      return;
    }
    const k = oldQuality / McradP.topLevelStochasticRadiosityElement(patch).quality;

    McradP.getTopLevelPatchRad(patch)![0]!.subtract(
      McradP.getTopLevelPatchRad(patch)![0]!,
      McradP.topLevelStochasticRadiosityElement(patch).sourceRad
    );

    Coefficientsmcrad.stochasticRadiosityScaleCoefficients(k, McradP.getTopLevelPatchRad(patch), McradP.getTopLevelPatchBasis(patch));
    Coefficientsmcrad.stochasticRadiosityScaleCoefficients(
      (1.0 - k),
      McradP.getTopLevelPatchReceivedRad(patch),
      McradP.getTopLevelPatchBasis(patch)
    );
    Coefficientsmcrad.stochasticRadiosityAddCoefficients(
      McradP.getTopLevelPatchRad(patch),
      McradP.getTopLevelPatchReceivedRad(patch),
      McradP.getTopLevelPatchBasis(patch)
    );

    McradP.getTopLevelPatchRad(patch)![0]!.add(
      McradP.getTopLevelPatchRad(patch)![0]!,
      McradP.topLevelStochasticRadiosityElement(patch).sourceRad
    );

    Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchUnShotRad(patch), McradP.getTopLevelPatchBasis(patch));
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));
  }

  private static randomWalkRadiosityDoShootingIteration(
    sceneWorldVoxelGrid: VoxelGrid,
    scenePatches: ArrayList<Patch>
  ): void {
    let numberOfWalks = StochasticRelaxation.activeState().initialNumberOfRays;
    const approx = (StochasticRelaxation.activeState().approximationOrderType
      ?? StochasticRaytracingApproximation.CONSTANT) as number;
    if (StochasticRelaxation.activeState().continuousRandomWalk !== 0) {
      numberOfWalks *= StochasticRadiosityBasisState.activeState().approxDesc[approx]!.basis_size;
    }
    else {
      numberOfWalks *= globalThis.Math.trunc(globalThis.Math.pow(
        StochasticRadiosityBasisState.activeState().approxDesc[approx]!.basis_size,
        1.0 / (1.0 - Statistics.instance().radiance.averageReflectivity.maximumComponent())
      ));
    }

    System.err.printf(
      "Shooting iteration %d (%d paths, approximately %d rays)\n",
      StochasticRelaxation.activeState().currentIteration,
      numberOfWalks,
      globalThis.Math.floor(numberOfWalks / (1.0 - Statistics.instance().radiance.averageReflectivity.maximumComponent()))
    );

    Tracepath.tracePaths(
      sceneWorldVoxelGrid,
      numberOfWalks,
      RandomWalkRadianceMethod.randomWalkRadiosityScalarSourcePower,
      RandomWalkRadianceMethod.randomWalkRadiosityScalarReflectance,
      RandomWalkRadianceMethod.randomWalkRadiosityShootingScore,
      RandomWalkRadianceMethod.randomWalkRadiosityShootingUpdate,
      scenePatches
    );
  }

  /**
Determines control radiosity value for collision gathering estimator
*/
  private static randomWalkRadiosityDetermineGatheringControlRadiosity(scenePatches: ArrayList<Patch>): ColorRgb {
    const c1 = new ColorRgb();
    const c2 = new ColorRgb();
    const cr = new ColorRgb();

    c1.clear();
    c2.clear();

    for (let i = 0; scenePatches !== null && i < scenePatches.size(); i++) {
      const patch = scenePatches.get(i);
      const absorb = new ColorRgb();
      const rho = McradP.topLevelStochasticRadiosityElement(patch).Rd;
      const Ed = McradP.topLevelStochasticRadiosityElement(patch).sourceRad;
      const num = new ColorRgb();
      const denominator = new ColorRgb();

      absorb.setMonochrome(1.0);
      absorb.subtract(absorb, rho);

      num.scalarProduct(absorb, Ed);
      c1.addScaled(c1, patch.area, num);

      denominator.scalarProduct(absorb, absorb);
      c2.addScaled(c2, patch.area, denominator);
    }

    cr.divide(c1, c2);
    System.err.printf("Control radiosity value = ");
    cr.print(System.err);
    System.err.printf(", luminosity = %g\n", Cie.spectrumLuminance(cr.r, cr.g, cr.b));

    return cr;
  }

  private static randomWalkRadiosityCollisionGatheringScore(
    path: Path,
    numberOfPaths: number,
    birthProbability: (patch: Patch) => number
  ): void {
    if (numberOfPaths < 0 || birthProbability === null) {
      // Keep C++ signature.
    }
    const lastNodeIndex = path.numberOfNodes - 1;
    let accumRad = new ColorRgb(
      McradP.topLevelStochasticRadiosityElement(path.nodes![lastNodeIndex]!.patch as Patch).sourceRad.r,
      McradP.topLevelStochasticRadiosityElement(path.nodes![lastNodeIndex]!.patch as Patch).sourceRad.g,
      McradP.topLevelStochasticRadiosityElement(path.nodes![lastNodeIndex]!.patch as Patch).sourceRad.b
    );
    for (let n = lastNodeIndex - 1; n >= 0; n--) {
      const node = path.nodes![n] as StochasticRaytracingPathNode;
      const uin = [0.0];
      const vin = [0.0];
      const uOut = [0.0];
      const vOut = [0.0];
      let r = 1.0;
      const patch = node.patch as Patch;
      const Rd = McradP.topLevelStochasticRadiosityElement(patch).Rd;
      accumRad.selfScalarProduct(Rd);

      patch.uniformUv(node.outpoint, uOut, vOut);
      if (StochasticRelaxation.activeState().continuousRandomWalk === 0) {
        r = 0.0;
        if (n > 0) {
          patch.uniformUv(node.inPoint, uin, vin);
        }
      }

      const basis = McradP.getTopLevelPatchBasis(patch) as GalerkinBasis;
      for (let i = 0; i < basis.size; i++) {
        const dual = basis.dualFunction![i]!(uOut[0]!, vOut[0]!);
        McradP.getTopLevelPatchReceivedRad(patch)![i]!.addScaled(
          McradP.getTopLevelPatchReceivedRad(patch)![i]!,
          dual,
          accumRad
        );

        if (StochasticRelaxation.activeState().continuousRandomWalk === 0) {
          const basf = basis.function![i]!(uin[0]!, vin[0]!);
          r += basf * dual;
        }
      }
      McradP.topLevelStochasticRadiosityElement(patch).ng++;

      accumRad.scale(r / node.probability);
      accumRad.add(accumRad, McradP.topLevelStochasticRadiosityElement(patch).sourceRad);
    }
  }

  private static randomWalkRadiosityGatheringUpdate(patch: Patch, w: number): void {
    if (w < -1) {
      // Keep C++ signature.
    }
    Coefficientsmcrad.stochasticRadiosityAddCoefficients(
      McradP.getTopLevelPatchUnShotRad(patch),
      McradP.getTopLevelPatchReceivedRad(patch),
      McradP.getTopLevelPatchBasis(patch)
    );
    Coefficientsmcrad.stochasticRadiosityCopyCoefficients(
      McradP.getTopLevelPatchRad(patch),
      McradP.getTopLevelPatchUnShotRad(patch),
      McradP.getTopLevelPatchBasis(patch)
    );

    if (McradP.topLevelStochasticRadiosityElement(patch).ng > 0) {
      Coefficientsmcrad.stochasticRadiosityScaleCoefficients(
        (1.0 / McradP.topLevelStochasticRadiosityElement(patch).ng),
        McradP.getTopLevelPatchRad(patch),
        McradP.getTopLevelPatchBasis(patch)
      );
    }

    McradP.getTopLevelPatchRad(patch)![0]!.add(
      McradP.getTopLevelPatchRad(patch)![0]!,
      McradP.topLevelStochasticRadiosityElement(patch).sourceRad
    );

    if (StochasticRelaxation.activeState().constantControlVariate !== 0) {
      const cr = new ColorRgb(
        StochasticRelaxation.activeState().controlRadiance.r,
        StochasticRelaxation.activeState().controlRadiance.g,
        StochasticRelaxation.activeState().controlRadiance.b
      );
      if (StochasticRelaxation.activeState().indirectOnly !== 0) {
        const Rd = McradP.topLevelStochasticRadiosityElement(patch).Rd;
        cr.scalarProduct(Rd, StochasticRelaxation.activeState().controlRadiance);
      }
      McradP.getTopLevelPatchRad(patch)![0]!.add(McradP.getTopLevelPatchRad(patch)![0]!, cr);
    }

    Coefficientsmcrad.stochasticRadiosityClearCoefficients(
      McradP.getTopLevelPatchReceivedRad(patch),
      McradP.getTopLevelPatchBasis(patch)
    );
  }

  private static randomWalkRadiosityDoGatheringIteration(
    sceneWorldVoxelGrid: VoxelGrid,
    scenePatches: ArrayList<Patch>
  ): void {
    let numberOfWalks = StochasticRelaxation.activeState().initialNumberOfRays;
    const approx = (StochasticRelaxation.activeState().approximationOrderType
      ?? StochasticRaytracingApproximation.CONSTANT) as number;
    if (StochasticRelaxation.activeState().continuousRandomWalk !== 0) {
      numberOfWalks *= StochasticRadiosityBasisState.activeState().approxDesc[approx]!.basis_size;
    }
    else {
      numberOfWalks *= globalThis.Math.trunc(globalThis.Math.pow(
        StochasticRadiosityBasisState.activeState().approxDesc[approx]!.basis_size,
        1.0 / (1.0 - Statistics.instance().radiance.averageReflectivity.maximumComponent())
      ));
    }

    if (StochasticRelaxation.activeState().constantControlVariate !== 0
      && StochasticRelaxation.activeState().currentIteration === 1) {
      StochasticRelaxation.activeState().controlRadiance =
        RandomWalkRadianceMethod.randomWalkRadiosityDetermineGatheringControlRadiosity(scenePatches);
      RandomWalkRadianceMethod.randomWalkRadiosityReduceSource(scenePatches);
    }

    System.err.printf(
      "Collision gathering iteration %d (%d paths, approximately %d rays)\n",
      StochasticRelaxation.activeState().currentIteration,
      numberOfWalks,
      globalThis.Math.floor(numberOfWalks / (1.0 - Statistics.instance().radiance.averageReflectivity.maximumComponent()))
    );

    Tracepath.tracePaths(
      sceneWorldVoxelGrid,
      numberOfWalks,
      RandomWalkRadianceMethod.randomWalkRadiosityPatchArea,
      RandomWalkRadianceMethod.randomWalkRadiosityScalarReflectance,
      RandomWalkRadianceMethod.randomWalkRadiosityCollisionGatheringScore,
      RandomWalkRadianceMethod.randomWalkRadiosityGatheringUpdate,
      scenePatches
    );
  }

  private static randomWalkRadiosityUpdateSourceIllumination(elem: StochasticRadiosityElement, w: number): void {
    if (w < -1) {
      // Keep C++ signature.
    }
    Coefficientsmcrad.stochasticRadiosityCopyCoefficients(elem.radiance, elem.receivedRadiance, elem.basis);
    elem.sourceRad.set(elem.receivedRadiance![0]!.r, elem.receivedRadiance![0]!.g, elem.receivedRadiance![0]!.b);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.unShotRadiance, elem.basis);
    Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
  }

  private static randomWalkRadiosityDoFirstShot(
    sceneWorldVoxelGrid: VoxelGrid,
    scenePatches: ArrayList<Patch>,
    renderOptions: RendererConfiguration
  ): void {
    const approx = (StochasticRelaxation.activeState().approximationOrderType
      ?? StochasticRaytracingApproximation.CONSTANT) as number;
    const numberOfRays = StochasticRelaxation.activeState().initialNumberOfRays *
      StochasticRadiosityBasisState.activeState().approxDesc[approx]!.basis_size;

    System.err.printf("First shot (%d rays):\n", numberOfRays);
    StochasticJacobi.doStochasticJacobiIteration(
      sceneWorldVoxelGrid,
      numberOfRays,
      RandomWalkRadianceMethod.randomWalkRadiosityGetSelfEmittedRadiance,
      null,
      RandomWalkRadianceMethod.randomWalkRadiosityUpdateSourceIllumination,
      scenePatches,
      renderOptions
    );
    RandomWalkRadianceMethod.randomWalkRadiosityPrintStats();
  }

  public override terminate(scenePatches: Patch[]): void {
    StochasticRelaxation.setActiveState(this.stochasticRelaxationState);
    ElementHierarchyState.setActiveState(this.elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(this.stochasticRadiosityBasisState);
    Mcrad.monteCarloRadiosityTerminate(RandomWalkRadianceMethod.toArrayList(scenePatches));
  }

  public override doStep(scene: Scene, renderOptions: RendererConfiguration): boolean {
    StochasticRelaxation.setActiveState(this.stochasticRelaxationState);
    ElementHierarchyState.setActiveState(this.elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(this.stochasticRadiosityBasisState);
    Mcrad.monteCarloRadiosityPreStep(scene, renderOptions);

    const scenePatches = RandomWalkRadianceMethod.toArrayList(scene.patchList);

    if (StochasticRelaxation.activeState().currentIteration === 1
      && StochasticRelaxation.activeState().indirectOnly !== 0) {
      RandomWalkRadianceMethod.randomWalkRadiosityDoFirstShot(scene.voxelGrid as VoxelGrid, scenePatches, renderOptions);
    }

    switch (StochasticRelaxation.activeState().randomWalkEstimatorType) {
      case RandomWalkEstimatorType.RW_SHOOTING:
        RandomWalkRadianceMethod.randomWalkRadiosityDoShootingIteration(scene.voxelGrid as VoxelGrid, scenePatches);
        break;
      case RandomWalkEstimatorType.RW_GATHERING:
        RandomWalkRadianceMethod.randomWalkRadiosityDoGatheringIteration(scene.voxelGrid as VoxelGrid, scenePatches);
        break;
      default:
        VsdkLogger.fatal(
          -1,
          "randomWalkRadiosityDoStep",
          "Unknown random walk estimator type %d",
          StochasticRelaxation.activeState().randomWalkEstimatorType as number
        );
        break;
    }

    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      Mcrad.monteCarloRadiosityPatchComputeNewColor(scene.patchList[i]!);
    }

    return false;
  }

  public override getStats(): string {
    StochasticRelaxation.setActiveState(this.stochasticRelaxationState);
    ElementHierarchyState.setActiveState(this.elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(this.stochasticRadiosityBasisState);

    const stats = new StringBuilder();
    const statsOffset = [0];

    RandomWalkRadianceMethod.appendRandomWalkStatsText(stats, statsOffset, "Random Walk Radiosity\nStatistics\n\n");
    RandomWalkRadianceMethod.appendRandomWalkStatsText(stats, statsOffset, "Iteration nr: %d\n", StochasticRelaxation.activeState().currentIteration);
    RandomWalkRadianceMethod.appendRandomWalkStatsText(stats, statsOffset, "CPU time: %g secs\n", StochasticRelaxation.activeState().cpuSeconds);
    RandomWalkRadianceMethod.appendRandomWalkStatsText(stats, statsOffset, "Radiance rays: %d\n", StochasticRelaxation.activeState().tracedRays);
    RandomWalkRadianceMethod.appendRandomWalkStatsText(stats, statsOffset, "Importance rays: %d\n", StochasticRelaxation.activeState().importanceTracedRays);

    return stats.toString();
  }
}
