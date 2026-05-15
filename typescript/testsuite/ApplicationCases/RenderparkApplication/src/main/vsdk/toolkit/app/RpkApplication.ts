import { OptionsGroupCore } from "./options/OptionsGroupCore";
import { GalerkinRadianceMethod } from "vitral/dist/vsdk/toolkit/galerkin/GalerkinRadianceMethod";
import { ClusterCreationStrategy } from "vitral/dist/vsdk/toolkit/galerkin/processing/ClusterCreationStrategy";
import { ParseRuntimeContext } from "vitral/dist/vsdk/toolkit/io/context/ParseRuntimeContext";
import { DkColor } from "vitral/dist/vsdk/toolkit/io/image/DkColor";
import { BidirectionalPathTracingState } from "vitral/dist/vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState";
import { LightList } from "vitral/dist/vsdk/toolkit/raycasting/bidirectionalRaytracing/LightList";
import { RayTracer } from "vitral/dist/vsdk/toolkit/raycasting/common/RayTracer";
import { PhotonMapConfig } from "vitral/dist/vsdk/toolkit/raycasting/photonMap/PhotonMapConfig";
import { PhotonMapState } from "vitral/dist/vsdk/toolkit/raycasting/photonMap/PhotonMapState";
import { RayMatterState } from "vitral/dist/vsdk/toolkit/raycasting/simple/RayMatterState";
import { ElementHierarchyState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState";
import { StochasticRadiosityBasisState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityBasisState";
import { StochasticRayTracingState } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRayTracingState";
import { StochasticRelaxation } from "vitral/dist/vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation";
import { PatchClusterOctreeNode } from "vitral/dist/vsdk/toolkit/scene/PatchClusterOctreeNode";
import { RadianceMethod } from "vitral/dist/vsdk/toolkit/scene/RadianceMethod";
import { Scene } from "vitral/dist/vsdk/toolkit/scene/Scene";
import { VoxelGrid } from "vitral/dist/vsdk/toolkit/scene/VoxelGrid";
import { Vertex } from "vitral/dist/vsdk/toolkit/environment/geometry/elements/Vertex";
import { VertexCompareFlags } from "vitral/dist/vsdk/toolkit/environment/geometry/elements/VertexCompareFlags";
import { FerwerdaToneMap } from "vitral/dist/vsdk/toolkit/tonemap/FerwerdaToneMap";
import { LightnessToneMap } from "vitral/dist/vsdk/toolkit/tonemap/LightnessToneMap";
import { RevisedTumblinRushmeierToneMap } from "vitral/dist/vsdk/toolkit/tonemap/RevisedTumblinRushmeierToneMap";
import { ToneMap } from "vitral/dist/vsdk/toolkit/tonemap/ToneMap";
import { ToneMappingContext } from "vitral/dist/vsdk/toolkit/tonemap/ToneMappingContext";
import { TumblinRushmeierToneMap } from "vitral/dist/vsdk/toolkit/tonemap/TumblinRushmeierToneMap";
import { WardToneMap } from "vitral/dist/vsdk/toolkit/tonemap/WardToneMap";
import { RendererConfiguration } from "vitral/dist/vsdk/toolkit/material/RendererConfiguration";
import { Material } from "vitral/dist/vsdk/toolkit/material/Material";
import { MgfParserLoader } from "vitral/dist/vsdk/toolkit/io/mgf/MgfParserLoader";
import { Batch } from "./Batch";
import { Radiance } from "./Radiance";
import { Raytrace } from "./Raytrace";
import { SceneBuilder } from "./SceneBuilder";

export class RpkApplication {
  private static readonly DEFAULT_MONOCHROME = false;
  private static readonly defaultMaterial = new Material(
    "(default)",
    null as unknown as any,
    null as unknown as any,
    false
  );
  private imageOutputWidth: number;
  private imageOutputHeight: number;
  private scene: Scene;
  private mgfContext: ParseRuntimeContext;
  private selectedRadianceMethod: RadianceMethod | null;
  private selectedToneMap: ToneMap | null;
  private toneMapOptions: ToneMappingContext;
  private renderOptions: RendererConfiguration;
  private rayTracer: RayTracer | null;
  private glutDebugEnabled: boolean;

  public constructor() {
    this.imageOutputWidth = 0;
    this.imageOutputHeight = 0;
    this.selectedRadianceMethod = null;
    this.selectedToneMap = null;
    this.toneMapOptions = new ToneMappingContext();
    this.rayTracer = null;
    this.glutDebugEnabled = false;

    this.scene = new Scene();
    this.mgfContext = new ParseRuntimeContext();
    this.renderOptions = new RendererConfiguration();
  }

  /**
  Global initializations
  */
  private static mainInitApplication(): void {
    // Default vertex compare flags: both location and normal is relevant. Two
    // vertices without normal, but at the same location, are to be considered
    // different
    Vertex.setCompareFlags(VertexCompareFlags.VERTEX_COMPARE_LOCATION | VertexCompareFlags.VERTEX_COMPARE_NORMAL);
  }

  private selectToneMapByName(name: string): void {
    let newMap: ToneMap;

    if (name === "TumblinRushmeier") {
      newMap = new TumblinRushmeierToneMap();
    }
    else if (name === "Ward") {
      newMap = new WardToneMap();
    }
    else if (name === "RevisedTR") {
      newMap = new RevisedTumblinRushmeierToneMap();
    }
    else if (name === "Ferwerda") {
      newMap = new FerwerdaToneMap();
    }
    else {
      newMap = new LightnessToneMap();
    }

    this.selectedToneMap = newMap;
    ToneMap.setActiveToneMap(this.selectedToneMap);
    newMap.init(this.toneMapOptions);
  }

  /**
  Processes command line arguments
  */
  private mainParseOptions(
    argc: number[],
    argv: string[],
    rayTracerName: string[],
    toneMapName: string[],
    stochasticRelaxationState: StochasticRelaxation,
    elementHierarchyState: ElementHierarchyState,
    stochasticRadiosityBasisState: StochasticRadiosityBasisState,
    photonMapState: PhotonMapState,
    photonMapConfig: PhotonMapConfig,
    rayMatterState: RayMatterState,
    bidirectionalPathState: BidirectionalPathTracingState,
    stochasticRayTracingState: StochasticRayTracingState
  ): void {
    const widthRef = [this.imageOutputWidth];
    const heightRef = [this.imageOutputHeight];
    const glutDebugRef = [this.glutDebugEnabled];

    OptionsGroupCore.parse(
      argc,
      argv,
      this.mgfContext,
      this.scene,
      this.renderOptions,
      this.toneMapOptions,
      widthRef,
      heightRef,
      glutDebugRef,
      toneMapName
    );
    this.imageOutputWidth = widthRef[0];
    this.imageOutputHeight = heightRef[0];
    this.glutDebugEnabled = glutDebugRef[0];

    const selectedMethodRef = [this.selectedRadianceMethod] as Array<RadianceMethod | null>;
    Radiance.radianceParseOptions(
      argc,
      argv,
      selectedMethodRef,
      stochasticRelaxationState,
      elementHierarchyState,
      stochasticRadiosityBasisState,
      photonMapState,
      photonMapConfig,
      rayMatterState,
      bidirectionalPathState,
      stochasticRayTracingState
    );
    this.selectedRadianceMethod = selectedMethodRef[0];

    Raytrace.rayTraceParseOptions(argc, argv, rayTracerName);

    Batch.generalParseOptions(argc, argv);
  }

  private mainCreateOffscreenCanvasWindow(): void {
    // Set correct outputImageWidth and outputImageHeight for the camera
    if (this.scene.camera !== null) {
      this.scene.camera.xSize = this.imageOutputWidth;
      this.scene.camera.ySize = this.imageOutputHeight;
    }
  }

  private executeRendering(
    rayTracerName: string,
    rayMatterState: RayMatterState,
    bidirectionalPathState: BidirectionalPathTracingState,
    stochasticRayTracingState: StochasticRayTracingState,
    lightList: Array<LightList | null>
  ): void {
    // Create the window in which to render (canvas window)
    this.mainCreateOffscreenCanvasWindow();

    this.rayTracer = Raytrace.rayTraceCreate(
      this.scene,
      this.toneMapOptions,
      rayTracerName,
      rayMatterState,
      bidirectionalPathState,
      stochasticRayTracingState,
      lightList
    );

    Batch.batchExecuteRadianceSimulation(this.scene, this.selectedRadianceMethod, this.rayTracer, this.toneMapOptions, this.renderOptions);
  }

  private static freeMemory(mgfContext: ParseRuntimeContext): void {
    MgfParserLoader.mgfFreeMemory(mgfContext);
    GalerkinRadianceMethod.freeMemory();
    PatchClusterOctreeNode.deleteCachedGeometries();
    ClusterCreationStrategy.freeClusterElements();
    VoxelGrid.freeVoxelGridElements();
    if (mgfContext.radianceMethod !== null) {
      mgfContext.radianceMethod = null;
      mgfContext.parserConfig.radianceMethod = null;
    }
    DkColor.freeBuffer();
  }

  public entryPoint(argc: number, argv: string[]): number {
    // 1. Default empty scene
    RpkApplication.mainInitApplication();

    const rayMatterState = new RayMatterState();
    const bidirectionalPathState = new BidirectionalPathTracingState();
    const stochasticRayTracingState = new StochasticRayTracingState();
    const stochasticRelaxationState = new StochasticRelaxation();
    const elementHierarchyState = new ElementHierarchyState();
    const stochasticRadiosityBasisState = new StochasticRadiosityBasisState();
    const photonMapState = new PhotonMapState();
    const photonMapConfig = new PhotonMapConfig();
    const lightList: Array<LightList | null> = [null];
    StochasticRelaxation.setActiveState(stochasticRelaxationState);
    ElementHierarchyState.setActiveState(elementHierarchyState);
    StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);

    // 2. Set model elements from command line options
    const rayTracerName = ["none"];
    const initializationToneMapName = ["Lightness"];
    const renderToneMapName = ["Lightness"];
    const argcRef = [argc];
    this.mainParseOptions(
      argcRef,
      argv,
      rayTracerName,
      renderToneMapName,
      stochasticRelaxationState,
      elementHierarchyState,
      stochasticRadiosityBasisState,
      photonMapState,
      photonMapConfig,
      rayMatterState,
      bidirectionalPathState,
      stochasticRayTracingState
    );

    // 3. Load scene elements from MGF file
    this.mgfContext.radianceMethod = this.selectedRadianceMethod;
    this.mgfContext.parserConfig.radianceMethod = this.selectedRadianceMethod;
    this.mgfContext.monochrome = RpkApplication.DEFAULT_MONOCHROME;
    this.mgfContext.parserConfig.monochrome = RpkApplication.DEFAULT_MONOCHROME;
    this.mgfContext.currentMaterial = RpkApplication.defaultMaterial;
    this.mgfContext.materialState.currentMaterial = RpkApplication.defaultMaterial;
    this.selectToneMapByName(initializationToneMapName[0]); // Note this is used for basic Galerkin model initialization
    SceneBuilder.sceneBuilderCreateModel(argcRef, argv, this.mgfContext, this.scene, this.toneMapOptions);
    this.selectToneMapByName(renderToneMapName[0]);

    // 4. Run main radiosity simulation and export result
    this.executeRendering(
      rayTracerName[0],
      rayMatterState,
      bidirectionalPathState,
      stochasticRayTracingState,
      lightList
    );

    // X. Interactive visual debug GUI tool (JOGL + Swing)
    if (this.glutDebugEnabled) {
      process.stderr.write("Glut debug tools are not available in the TypeScript build (render without OpenGL).\n");
    }

    // 5. Free used memory
    RpkApplication.freeMemory(this.mgfContext);

    if (this.rayTracer !== null) {
      this.rayTracer.terminate();
      this.rayTracer = null;
    }
    if (lightList[0] !== null) {
      lightList[0] = null;
    }

    return 0;
  }
}
