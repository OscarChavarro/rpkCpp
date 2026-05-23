import { OutputStream } from "../../../java/io/OutputStream";
import { ColorRgb } from "../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../common/logging/Logger";
import { RendererConfiguration } from "../material/RendererConfiguration";
import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { Statistics } from "../common/statistics/Statistics";
import { VrmlWriter } from "../io/wrl/VrmlWriter";
import { BsdfComponent } from "../material/BsdfComponent";
import { XxdfComponentFlag } from "../material/XxdfComponentFlag";
import { PatchVisitor } from "../numericalAnalysis/PatchVisitor";
import { ClusterCreationStrategy } from "./processing/ClusterCreationStrategy";
import { GatheringClusteredStrategy } from "./processing/GatheringClusteredStrategy";
import { GatheringSimpleStrategy } from "./processing/GatheringSimpleStrategy";
import { GatheringStrategy } from "./processing/GatheringStrategy";
import { ScratchVisibilityStrategy } from "./processing/ScratchVisibilityStrategy";
import { ShootingStrategy } from "./processing/ShootingStrategy";
import { Camera } from "../scene/Camera";
import { RadianceMethod } from "../scene/RadianceMethod";
import { RadianceMethodAlgorithm } from "../scene/RadianceMethodAlgorithm";
import { Scene } from "../scene/Scene";
import { Element } from "../environment/geometry/elements/Element";
import { Patch } from "../environment/geometry/elements/Patch";
import { ToneMap } from "../tonemap/ToneMap";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { GalerkinBasis } from "./GalerkinBasis";
import { GalerkinClusteringStrategy } from "./GalerkinClusteringStrategy";
import { GalerkinElement } from "./GalerkinElement";
import { GalerkinIterationMethod } from "./GalerkinIterationMethod";
import { GalerkinState } from "./GalerkinState";
import { Interaction } from "./Interaction";

export class GalerkinRadianceMethod extends RadianceMethod {
  private static readonly STRING_LENGTH = 2000;
  private static readonly ALL_XXDF_COMPONENTS =
    XxdfComponentFlag.DIFFUSE_COMPONENT
    | XxdfComponentFlag.GLOSSY_COMPONENT
    | XxdfComponentFlag.SPECULAR_COMPONENT;
  private static readonly ALL_BSDF_COMPONENTS =
    BsdfComponent.BRDF_DIFFUSE_COMPONENT
    | BsdfComponent.BRDF_GLOSSY_COMPONENT
    | BsdfComponent.BRDF_SPECULAR_COMPONENT
    | BsdfComponent.BTDF_DIFFUSE_COMPONENT
    | BsdfComponent.BTDF_GLOSSY_COMPONENT
    | BsdfComponent.BTDF_SPECULAR_COMPONENT;

  private gatheringStrategy: GatheringStrategy | null;
  private static vrmlOutputStream: OutputStream | null = null;
  private static numberOfWrites = 0;
  private static vertexId = 0;

  public static galerkinState = new GalerkinState();

  public static freeMemory(): void {
    if (GalerkinRadianceMethod.galerkinState.scratch !== null) {
      GalerkinRadianceMethod.galerkinState.scratch = null;
    }
  }

  public static recomputePatchColor(patch: Patch): void {
    if (patch === null || !(patch.radianceData instanceof GalerkinElement)) {
      return;
    }

    const element = patch.radianceData as GalerkinElement;
    const reflectivity = element.Rd;
    const radVis = new ColorRgb();

    if (GalerkinRadianceMethod.galerkinState.useAmbientRadiance !== 0) {
      radVis.scalarProduct(reflectivity, GalerkinRadianceMethod.galerkinState.ambientRadiance);
      radVis.add(radVis, (element.radiance as ColorRgb[])[0]!);
      ToneMap.radianceToRgb(
        radVis,
        patch.color,
        GalerkinRadianceMethod.galerkinState.toneMapOptions as ToneMappingContext,
      );
    }
    else {
      ToneMap.radianceToRgb(
        (element.radiance as ColorRgb[])[0]!,
        patch.color,
        GalerkinRadianceMethod.galerkinState.toneMapOptions as ToneMappingContext,
      );
    }
    patch.computeVertexColors();
  }

  public constructor() {
    super();
    this.className = RadianceMethodAlgorithm.GALERKIN;
    this.gatheringStrategy = null;
  }

  public override getRadianceMethodName(): string {
    return "Galerkin";
  }

  public setStrategy(): void {
    this.gatheringStrategy = null;
    if (GalerkinRadianceMethod.galerkinState.clustered !== 0) {
      this.gatheringStrategy = new GatheringClusteredStrategy();
    }
    else {
      this.gatheringStrategy = new GatheringSimpleStrategy();
    }
  }

  public override parseOptions(argc: number[], argv: string[]): void {
    if (argc !== null || argv !== null) {
      // Nothing to parse in this migration step.
    }
  }

  private static patchInit(patch: Patch): void {
    if (patch === null || !(patch.radianceData instanceof GalerkinElement)) {
      return;
    }

    const element = patch.radianceData as GalerkinElement;
    const reflectivity = element.Rd;
    const selfEmittanceRadiance = element.Ed;

    if (GalerkinRadianceMethod.galerkinState.useConstantRadiance !== 0) {
      (element.radiance as ColorRgb[])[0]!.scalarProduct(reflectivity, GalerkinRadianceMethod.galerkinState.constantRadiance);
      (element.radiance as ColorRgb[])[0]!.add((element.radiance as ColorRgb[])[0]!, selfEmittanceRadiance);
      if (GalerkinRadianceMethod.galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
        (element.unShotRadiance as ColorRgb[])[0]!.subtract(
          (element.radiance as ColorRgb[])[0]!,
          GalerkinRadianceMethod.galerkinState.constantRadiance,
        );
      }
    }
    else {
      (element.radiance as ColorRgb[])[0]!.set(selfEmittanceRadiance.r, selfEmittanceRadiance.g, selfEmittanceRadiance.b);
      if (GalerkinRadianceMethod.galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
        (element.unShotRadiance as ColorRgb[])[0]!.set(
          (element.radiance as ColorRgb[])[0]!.r,
          (element.radiance as ColorRgb[])[0]!.g,
          (element.radiance as ColorRgb[])[0]!.b,
        );
      }
    }

    if (GalerkinRadianceMethod.galerkinState.importanceDriven !== 0) {
      switch (GalerkinRadianceMethod.galerkinState.galerkinIterationMethod) {
        case GalerkinIterationMethod.GAUSS_SEIDEL:
        case GalerkinIterationMethod.JACOBI:
          element.potential = patch.directPotential;
          break;
        case GalerkinIterationMethod.SOUTH_WELL:
          element.potential = patch.directPotential;
          element.unShotPotential = patch.directPotential;
          break;
        default:
          VsdkLogger.fatal(-1, "patchInit", "Invalid iteration method");
      }
    }

    GalerkinRadianceMethod.recomputePatchColor(patch);
  }

  public override initialize(scene: Scene, toneMapOptions: ToneMappingContext): void {
    GalerkinRadianceMethod.galerkinState.toneMapOptions = toneMapOptions;
    if (GalerkinRadianceMethod.galerkinState.toneMapOptions === null) {
      VsdkLogger.fatal(-1, "GalerkinRadianceMethod::initialize", "Tone mapping context not provided");
    }

    GalerkinRadianceMethod.galerkinState.iterationNumber = 0;
    GalerkinRadianceMethod.galerkinState.cpuSeconds = 0.0;

    GalerkinElement.initializeBasis();

    GalerkinRadianceMethod.galerkinState.constantRadiance = Statistics.instance().radiance.estimatedAverageRadiance;
    if (GalerkinRadianceMethod.galerkinState.useConstantRadiance !== 0) {
      GalerkinRadianceMethod.galerkinState.ambientRadiance.clear();
    }
    else {
      GalerkinRadianceMethod.galerkinState.ambientRadiance = Statistics.instance().radiance.estimatedAverageRadiance;
    }

    for (let i = 0; scene.patchList !== null && i < scene.patchList.length; i++) {
      GalerkinRadianceMethod.patchInit(scene.patchList[i]!);
    }

    GalerkinRadianceMethod.galerkinState.topCluster = ClusterCreationStrategy.createClusterHierarchy(
      scene.clusteredRootGeometry,
      GalerkinRadianceMethod.galerkinState,
    );

    if (GalerkinRadianceMethod.galerkinState.clusteringStrategy === GalerkinClusteringStrategy.Z_VISIBILITY) {
      ScratchVisibilityStrategy.scratchInit(GalerkinRadianceMethod.galerkinState);
    }

    GalerkinRadianceMethod.galerkinState.lastClusterId = -1;
    GalerkinRadianceMethod.galerkinState.lastEye.set(
      Numeric.HUGE_FLOAT_VALUE,
      Numeric.HUGE_FLOAT_VALUE,
      Numeric.HUGE_FLOAT_VALUE,
    );
  }

  public override doStep(scene: Scene, renderOptions: RendererConfiguration): boolean {
    if (GalerkinRadianceMethod.galerkinState.iterationNumber < 0) {
      VsdkLogger.error("doGalerkinOneStep", "method not initialized");
      return true;
    }

    GalerkinRadianceMethod.galerkinState.iterationNumber++;
    GalerkinRadianceMethod.galerkinState.lastClock = process.hrtime.bigint();

    let done: boolean;
    switch (GalerkinRadianceMethod.galerkinState.galerkinIterationMethod) {
      case GalerkinIterationMethod.JACOBI:
      case GalerkinIterationMethod.GAUSS_SEIDEL:
        if (this.gatheringStrategy === null) {
          this.setStrategy();
        }
        done = (this.gatheringStrategy as GatheringStrategy).doGatheringIteration(
          scene,
          GalerkinRadianceMethod.galerkinState,
          renderOptions,
        );
        break;
      case GalerkinIterationMethod.SOUTH_WELL:
        done = ShootingStrategy.doShootingStep(scene, GalerkinRadianceMethod.galerkinState, renderOptions);
        break;
      default:
        VsdkLogger.fatal(
          2,
          "doGalerkinOneStep",
          "Invalid iteration method %s\n",
          GalerkinRadianceMethod.galerkinState.galerkinIterationMethod,
        );
        done = true;
        break;
    }

    GalerkinRadianceMethod.updateCpuSecs();
    return done;
  }

  private static galerkinDestroyClusterHierarchy(clusterElement: GalerkinElement): void {
    if (clusterElement === null || !clusterElement.isCluster()) {
      return;
    }

    for (let i = 0;
      clusterElement.irregularSubElements !== null && i < clusterElement.irregularSubElements.length;
      i++) {
      const child = clusterElement.irregularSubElements[i];
      if (child instanceof GalerkinElement) {
        GalerkinRadianceMethod.galerkinDestroyClusterHierarchy(child as GalerkinElement);
      }
    }
  }

  private static regularSubdivisionDepth(element: GalerkinElement): number {
    if (element === null || element.regularSubElements === null) {
      return 0;
    }
    let maxChildDepth = 0;
    for (let i = 0; i < 4; i++) {
      if (element.regularSubElements[i] instanceof GalerkinElement) {
        const childDepth = GalerkinRadianceMethod.regularSubdivisionDepth(
          element.regularSubElements[i] as GalerkinElement,
        );
        if (childDepth > maxChildDepth) {
          maxChildDepth = childDepth;
        }
      }
    }
    return 1 + maxChildDepth;
  }

  public override terminate(scenePatches: Patch[]): void {
    if (GalerkinRadianceMethod.galerkinState.clusteringStrategy === GalerkinClusteringStrategy.Z_VISIBILITY) {
      ScratchVisibilityStrategy.scratchTerminate(GalerkinRadianceMethod.galerkinState);
    }

    if (scenePatches !== null) {
      for (let i = 0; i < scenePatches.length; i++) {
        const patch = scenePatches[i]!;
        if (patch !== null) {
          GalerkinRadianceMethod.recomputePatchColor(patch);
        }
      }
    }

    if (GalerkinRadianceMethod.galerkinState.topCluster !== null) {
      GalerkinRadianceMethod.galerkinDestroyClusterHierarchy(
        GalerkinRadianceMethod.galerkinState.topCluster,
      );
      GalerkinRadianceMethod.galerkinState.topCluster = null;
    }
    GalerkinRadianceMethod.galerkinState.iterationNumber = -1;
  }

  private static updateCpuSecs(): void {
    const t = process.hrtime.bigint();
    GalerkinRadianceMethod.galerkinState.cpuSeconds += Number(t - GalerkinRadianceMethod.galerkinState.lastClock) / 1_000_000_000.0;
    GalerkinRadianceMethod.galerkinState.lastClock = t;
  }

  private computePatchRadiance(patch: Patch, u: number, v: number): ColorRgb {
    if (patch === null) {
      const black = new ColorRgb();
      black.clear();
      return black;
    }

    if (patch.jacobian !== null) {
      const uu = [u];
      const vv = [v];
      patch.biLinearToUniform(uu, vv);
      u = uu[0]!;
      v = vv[0]!;
    }

    const topLevelElement = GalerkinElement.fromPatch(patch);
    if (topLevelElement === null) {
      const black = new ColorRgb();
      black.clear();
      return black;
    }
    const uu = [u];
    const vv = [v];
    const leaf = topLevelElement.regularLeafAtPoint(uu, vv);
    const rad = GalerkinBasis.radianceAtPoint(leaf, leaf.radiance, uu[0]!, vv[0]!);

    if (GalerkinRadianceMethod.galerkinState.useAmbientRadiance !== 0) {
      const reflectivity = patch.radianceData!.Rd;
      const ambientRadiance = new ColorRgb();
      ambientRadiance.scalarProduct(reflectivity, GalerkinRadianceMethod.galerkinState.ambientRadiance);
      rad.add(rad, ambientRadiance);
    }

    return rad;
  }

  public override getRadiance(
    camera: Camera,
    patch: Patch,
    u: number,
    v: number,
    dir: Vector3D,
    renderOptions: RendererConfiguration,
  ): ColorRgb {
    if (camera !== null || dir !== null || renderOptions !== null) {
      // Parameters intentionally unused in this method.
    }
    return this.computePatchRadiance(patch, u, v);
  }

  public override createPatchData(patch: Patch): Element {
    patch.radianceData = new GalerkinElement(patch, GalerkinRadianceMethod.galerkinState);
    return patch.radianceData;
  }

  public override destroyPatchData(patch: Patch): void {
    if (patch !== null) {
      patch.radianceData = null;
    }
  }

  public override getStats(): string {
    let stats = "";
    stats += "Galerkin Radiosity Statistics:\n\n";
    stats += `Iteration nr: ${GalerkinRadianceMethod.galerkinState.iterationNumber}\n`;
    stats += `Nr. elements: ${GalerkinElement.getNumberOfElements()}\n`;
    stats += `clusters: ${GalerkinElement.getNumberOfClusters()}\n`;
    stats += `surface elements: ${GalerkinElement.getNumberOfSurfaceElements()}\n`;
    stats += `Nr. interactions: ${Interaction.getNumberOfInteractions()}\n`;
    stats += `cluster to cluster: ${Interaction.getNumberOfClusterToClusterInteractions()}\n`;
    stats += `cluster to surface: ${Interaction.getNumberOfClusterToSurfaceInteractions()}\n`;
    stats += `surface to cluster: ${Interaction.getNumberOfSurfaceToClusterInteractions()}\n`;
    stats += `surface to surface: ${Interaction.getNumberOfSurfaceToSurfaceInteractions()}\n`;
    stats += `shadow hits: ${Statistics.instance().shadow.numberOfShadowRays}\n`;
    stats += `shadow hits cached: ${Statistics.instance().shadow.numberOfShadowCacheHits}\n`;
    stats += `CPU time: ${GalerkinRadianceMethod.galerkinState.cpuSeconds} secs\n`;
    stats += `Clustered: ${GalerkinRadianceMethod.galerkinState.clustered}\n`;
    stats += `Importance driven: ${GalerkinRadianceMethod.galerkinState.importanceDriven}\n`;
    return stats;
  }

  public override writeVRML(
    camera: Camera,
    outputStream: OutputStream,
    renderOptions: RendererConfiguration,
  ): void {
    if (camera === null || outputStream === null || renderOptions === null) {
      return;
    }

    VrmlWriter.writeHeader(camera, outputStream, renderOptions);
    GalerkinRadianceMethod.vrmlOutputStream = outputStream;
    GalerkinRadianceMethod.writeCoords();
    GalerkinRadianceMethod.writeColors(renderOptions);
    GalerkinRadianceMethod.writeCoordIndicesTopCluster();
    VrmlWriter.writeTrailer(outputStream);
  }

  private static formatArgument(conversion: string, argument: unknown): string {
    if (conversion === "s") {
      return `${argument ?? ""}`;
    }

    const numericValue = Number(argument);
    if (!Number.isFinite(numericValue)) {
      if (conversion === "d") {
        return "0";
      }
      return `${numericValue}`;
    }

    switch (conversion) {
      case "d":
        return `${numericValue < 0 ? globalThis.Math.ceil(numericValue) : globalThis.Math.floor(numericValue)}`;
      case "f":
        return numericValue.toFixed(6);
      case "g":
      default:
        return `${numericValue}`;
    }
  }

  private static formatToString(format: string | null, ...argumentsList: unknown[]): string {
    if (format === null) {
      return "";
    }

    let argumentIndex = 0;
    return format.replace(/%(?:%|[gsfd])/g, (token: string): string => {
      if (token === "%%") {
        return "%";
      }

      const conversion = token.charAt(1);
      if (argumentIndex >= argumentsList.length) {
        return token;
      }

      const replacement = GalerkinRadianceMethod.formatArgument(conversion, argumentsList[argumentIndex]);
      argumentIndex++;
      return replacement;
    });
  }

  private static writeFormatted(format: string | null, ...argumentsList: unknown[]): void {
    if (GalerkinRadianceMethod.vrmlOutputStream === null || format === null) {
      return;
    }

    let text = "";
    try {
      text = GalerkinRadianceMethod.formatToString(format, ...argumentsList);
    }
    catch (_ignored) {
      text = "";
    }

    if (text.length <= 0) {
      return;
    }

    const bytes = Buffer.from(text, "utf8");
    try {
      GalerkinRadianceMethod.vrmlOutputStream.write(bytes, 0, bytes.length);
    }
    catch (_ignored) {
    }
  }

  private static writeVertexCoord(p: Vector3D): void {
    if (p === null) {
      return;
    }
    if (GalerkinRadianceMethod.numberOfWrites > 0) {
      GalerkinRadianceMethod.writeFormatted("%s", ", ");
    }
    GalerkinRadianceMethod.numberOfWrites++;
    if (GalerkinRadianceMethod.numberOfWrites % 4 === 0) {
      GalerkinRadianceMethod.writeFormatted("%s", "\n\t  ");
    }
    GalerkinRadianceMethod.writeFormatted("%g %g %g", p.x, p.y, p.z);
    GalerkinRadianceMethod.vertexId++;
  }

  private static writeVertexCoords(element: Element): void {
    if (!(element instanceof GalerkinElement)) {
      return;
    }
    const galerkinElement = element as GalerkinElement;
    const v: Vector3D[] = [
      new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(),
      new Vector3D(), new Vector3D(), new Vector3D(), new Vector3D(),
    ];
    const numberOfVertices = galerkinElement.vertices(v);
    for (let i = 0; i < numberOfVertices; i++) {
      GalerkinRadianceMethod.writeVertexCoord(v[i]!);
    }
  }

  private static writeCoords(): void {
    if (GalerkinRadianceMethod.galerkinState.topCluster === null) {
      return;
    }
    GalerkinRadianceMethod.numberOfWrites = 0;
    GalerkinRadianceMethod.vertexId = 0;
    GalerkinRadianceMethod.writeFormatted("%s", "\tcoord Coordinate {\n\t  point [ ");
    GalerkinRadianceMethod.galerkinState.topCluster.traverseAllLeafElements(GalerkinRadianceMethod.writeVertexCoords);
    GalerkinRadianceMethod.writeFormatted("%s", " ] ");
    GalerkinRadianceMethod.writeFormatted("%s", "\n\t}\n");
  }

  private static writeVertexColor(color: ColorRgb): void {
    if (color === null) {
      return;
    }
    if (GalerkinRadianceMethod.numberOfWrites > 0) {
      GalerkinRadianceMethod.writeFormatted("%s", ", ");
    }
    GalerkinRadianceMethod.numberOfWrites++;
    if (GalerkinRadianceMethod.numberOfWrites % 4 === 0) {
      GalerkinRadianceMethod.writeFormatted("%s", "\n\t  ");
    }
    GalerkinRadianceMethod.writeFormatted("%.3g %.3g %.3g", color.r, color.g, color.b);
    GalerkinRadianceMethod.vertexId++;
  }

  private static writeVertexColors(element: Element): void {
    if (!(element instanceof GalerkinElement)) {
      return;
    }
    const galerkinElement = element as GalerkinElement;
    if (galerkinElement.patch === null) {
      return;
    }

    const vertexRadiosity: ColorRgb[] = [new ColorRgb(), new ColorRgb(), new ColorRgb(), new ColorRgb()];
    const numberOfVertices = galerkinElement.patch.numberOfVertices;
    if (numberOfVertices === 3) {
      vertexRadiosity[0] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 0.0, 0.0);
      vertexRadiosity[1] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 1.0, 0.0);
      vertexRadiosity[2] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 0.0, 1.0);
    }
    else {
      vertexRadiosity[0] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 0.0, 0.0);
      vertexRadiosity[1] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 1.0, 0.0);
      vertexRadiosity[2] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 1.0, 1.0);
      vertexRadiosity[3] = GalerkinBasis.radianceAtPoint(galerkinElement, galerkinElement.radiance, 0.0, 1.0);
    }

    if (GalerkinRadianceMethod.galerkinState.useAmbientRadiance !== 0) {
      const reflectivity = galerkinElement.patch.radianceData!.Rd;
      const ambient = new ColorRgb();
      ambient.scalarProduct(reflectivity, GalerkinRadianceMethod.galerkinState.ambientRadiance);
      for (let i = 0; i < numberOfVertices; i++) {
        vertexRadiosity[i]!.add(vertexRadiosity[i]!, ambient);
      }
    }

    for (let i = 0; i < numberOfVertices; i++) {
      const col = new ColorRgb();
      ToneMap.radianceToRgb(
        vertexRadiosity[i]!,
        col,
        GalerkinRadianceMethod.galerkinState.toneMapOptions as ToneMappingContext,
      );
      GalerkinRadianceMethod.writeVertexColor(col);
    }
  }

  private static writeVertexColorsTopCluster(): void {
    if (GalerkinRadianceMethod.galerkinState.topCluster === null) {
      return;
    }
    GalerkinRadianceMethod.vertexId = 0;
    GalerkinRadianceMethod.numberOfWrites = 0;
    GalerkinRadianceMethod.writeFormatted("%s", "\tcolor Color {\n\t  color [ ");
    GalerkinRadianceMethod.galerkinState.topCluster.traverseAllLeafElements(GalerkinRadianceMethod.writeVertexColors);
    GalerkinRadianceMethod.writeFormatted("%s", " ] ");
    GalerkinRadianceMethod.writeFormatted("%s", "\n\t}\n");
  }

  private static writeColors(renderOptions: RendererConfiguration): void {
    if (renderOptions === null) {
      return;
    }
    if (!renderOptions.smoothShading) {
      VsdkLogger.warning(null, "I assume you want a smooth shaded model ...");
    }
    GalerkinRadianceMethod.writeFormatted("\tcolorPerVertex %s\n", "TRUE");
    GalerkinRadianceMethod.writeVertexColorsTopCluster();
  }

  private static writeCoordIndex(index: number): void {
    GalerkinRadianceMethod.numberOfWrites++;
    if (GalerkinRadianceMethod.numberOfWrites % 20 === 0) {
      GalerkinRadianceMethod.writeFormatted("%s", "\n\t  ");
    }
    GalerkinRadianceMethod.writeFormatted("%d ", index);
  }

  private static writeCoordIndices(element: Element): void {
    if (!(element instanceof GalerkinElement)) {
      return;
    }
    const galerkinElement = element as GalerkinElement;
    if (galerkinElement.patch === null) {
      return;
    }
    for (let i = 0; i < galerkinElement.patch.numberOfVertices; i++) {
      GalerkinRadianceMethod.writeCoordIndex(GalerkinRadianceMethod.vertexId);
      GalerkinRadianceMethod.vertexId++;
    }
    GalerkinRadianceMethod.writeCoordIndex(-1);
  }

  private static writeCoordIndicesTopCluster(): void {
    if (GalerkinRadianceMethod.galerkinState.topCluster === null) {
      return;
    }
    GalerkinRadianceMethod.vertexId = 0;
    GalerkinRadianceMethod.numberOfWrites = 0;
    GalerkinRadianceMethod.writeFormatted("%s", "\tcoordIndex [ ");
    GalerkinRadianceMethod.galerkinState.topCluster.traverseAllLeafElements(GalerkinRadianceMethod.writeCoordIndices);
    GalerkinRadianceMethod.writeFormatted("%s", " ]\n");
  }
}
