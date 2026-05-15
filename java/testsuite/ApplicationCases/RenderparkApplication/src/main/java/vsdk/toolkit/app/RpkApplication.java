package vsdk.toolkit.app;

import vsdk.toolkit.app.options.OptionsGroupCore;
import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import vsdk.toolkit.galerkin.processing.ClusterCreationStrategy;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.image.DkColor;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.BidirectionalPathTracingState;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.LightList;
import vsdk.toolkit.raycasting.common.RayTracer;
import vsdk.toolkit.raycasting.photonMap.PhotonMapConfig;
import vsdk.toolkit.raycasting.photonMap.PhotonMapState;
import vsdk.toolkit.raycasting.simple.RayMatterState;
import vsdk.toolkit.raycasting.stochasticRaytracing.ElementHierarchyState;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRadiosityBasisState;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRayTracingState;
import vsdk.toolkit.raycasting.stochasticRaytracing.StochasticRelaxation;
import vsdk.toolkit.render.jogl.visualDebugTools.GlutDebugState;
import vsdk.toolkit.render.jogl.visualDebugTools.GlutDebugTools;
import vsdk.toolkit.render.jogl.visualDebugTools.GlutDebugToolsModel;
import vsdk.toolkit.scene.PatchClusterOctreeNode;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.environment.geometry.elements.Vertex;
import vsdk.toolkit.environment.geometry.elements.VertexCompareFlags;
import vsdk.toolkit.tonemap.FerwerdaToneMap;
import vsdk.toolkit.tonemap.LightnessToneMap;
import vsdk.toolkit.tonemap.RevisedTumblinRushmeierToneMap;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;
import vsdk.toolkit.tonemap.TumblinRushmeierToneMap;
import vsdk.toolkit.tonemap.WardToneMap;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.io.mgf.MgfParserLoader;

public class RpkApplication {
    private static final boolean DEFAULT_MONOCHROME = false;
    private static final Material defaultMaterial = new Material("(default)", null, null, false);
    private int imageOutputWidth;
    private int imageOutputHeight;
    private Scene scene;
    private ParseRuntimeContext mgfContext;
    private RadianceMethod selectedRadianceMethod;
    private ToneMap selectedToneMap;
    private ToneMappingContext toneMapOptions;
    private RenderOptions renderOptions;
    private RayTracer rayTracer;
    private boolean glutDebugEnabled;

    public RpkApplication() {
        imageOutputWidth = 0;
        imageOutputHeight = 0;
        selectedRadianceMethod = null;
        selectedToneMap = null;
        toneMapOptions = new ToneMappingContext();
        rayTracer = null;
        glutDebugEnabled = false;

        scene = new Scene();
        mgfContext = new ParseRuntimeContext();
        renderOptions = new RenderOptions();
    }

    /**
Global initializations
*/
    private static void mainInitApplication() {
        // Default vertex compare flags: both location and normal is relevant. Two
        // vertices without normal, but at the same location, are to be considered
        // different
        Vertex.setCompareFlags(VertexCompareFlags.VERTEX_COMPARE_LOCATION | VertexCompareFlags.VERTEX_COMPARE_NORMAL);
    }

    private void selectToneMapByName(String name) {
        ToneMap newMap;

        if ( "TumblinRushmeier".equals(name) ) {
            newMap = new TumblinRushmeierToneMap();
        }
        else if ( "Ward".equals(name) ) {
            newMap = new WardToneMap();
        }
        else if ( "RevisedTR".equals(name) ) {
            newMap = new RevisedTumblinRushmeierToneMap();
        }
        else if ( "Ferwerda".equals(name) ) {
            newMap = new FerwerdaToneMap();
        }
        else {
            newMap = new LightnessToneMap();
        }

        selectedToneMap = newMap;
        ToneMap.setActiveToneMap(selectedToneMap);
        newMap.init(toneMapOptions);
    }

    /**
Processes command line arguments
*/
    private void mainParseOptions(
        int[] argc,
        String[] argv,
        String[] rayTracerName,
        String[] toneMapName,
        StochasticRelaxation stochasticRelaxationState,
        ElementHierarchyState elementHierarchyState,
        StochasticRadiosityBasisState stochasticRadiosityBasisState,
        PhotonMapState photonMapState,
        PhotonMapConfig photonMapConfig,
        RayMatterState rayMatterState,
        BidirectionalPathTracingState bidirectionalPathState,
        StochasticRayTracingState stochasticRayTracingState)
    {
        int[] widthRef = new int[] {imageOutputWidth};
        int[] heightRef = new int[] {imageOutputHeight};
        boolean[] glutDebugRef = new boolean[] {glutDebugEnabled};

        OptionsGroupCore.parse(
            argc,
            argv,
            mgfContext,
            scene,
            renderOptions,
            toneMapOptions,
            widthRef,
            heightRef,
            glutDebugRef,
            toneMapName);
        imageOutputWidth = widthRef[0];
        imageOutputHeight = heightRef[0];
        glutDebugEnabled = glutDebugRef[0];

        RadianceMethod[] selectedMethodRef = new RadianceMethod[] {selectedRadianceMethod};
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
            stochasticRayTracingState);
        selectedRadianceMethod = selectedMethodRef[0];

        Raytrace.rayTraceParseOptions(argc, argv, rayTracerName);

        Batch.generalParseOptions(argc, argv);
    }

    private void mainCreateOffscreenCanvasWindow() {
        // Set correct outputImageWidth and outputImageHeight for the camera
        scene.camera.xSize = imageOutputWidth;
        scene.camera.ySize = imageOutputHeight;
    }

    private void executeRendering(
        String rayTracerName,
        RayMatterState rayMatterState,
        BidirectionalPathTracingState bidirectionalPathState,
        StochasticRayTracingState stochasticRayTracingState,
        LightList[] lightList)
    {
        // Create the window in which to render (canvas window)
        mainCreateOffscreenCanvasWindow();

        rayTracer = Raytrace.rayTraceCreate(
            scene,
            toneMapOptions,
            rayTracerName,
            rayMatterState,
            bidirectionalPathState,
            stochasticRayTracingState,
            lightList);

        Batch.batchExecuteRadianceSimulation(scene, selectedRadianceMethod, rayTracer, toneMapOptions, renderOptions);
    }

    private static void freeMemory(ParseRuntimeContext mgfContext) {
        MgfParserLoader.mgfFreeMemory(mgfContext);
        GalerkinRadianceMethod.freeMemory();
        PatchClusterOctreeNode.deleteCachedGeometries();
        ClusterCreationStrategy.freeClusterElements();
        VoxelGrid.freeVoxelGridElements();
        if ( mgfContext.radianceMethod != null ) {
            mgfContext.radianceMethod = null;
            mgfContext.parserConfig.radianceMethod = null;
        }
        DkColor.freeBuffer();
    }

    public int entryPoint(int argc, String[] argv) {
        // 1. Default empty scene
        mainInitApplication();

        RayMatterState rayMatterState = new RayMatterState();
        BidirectionalPathTracingState bidirectionalPathState = new BidirectionalPathTracingState();
        StochasticRayTracingState stochasticRayTracingState = new StochasticRayTracingState();
        StochasticRelaxation stochasticRelaxationState = new StochasticRelaxation();
        ElementHierarchyState elementHierarchyState = new ElementHierarchyState();
        StochasticRadiosityBasisState stochasticRadiosityBasisState = new StochasticRadiosityBasisState();
        PhotonMapState photonMapState = new PhotonMapState();
        PhotonMapConfig photonMapConfig = new PhotonMapConfig();
        LightList[] lightList = new LightList[] {null};
        StochasticRelaxation.setActiveState(stochasticRelaxationState);
        ElementHierarchyState.setActiveState(elementHierarchyState);
        StochasticRadiosityBasisState.setActiveState(stochasticRadiosityBasisState);

        // 2. Set model elements from command line options
        String[] rayTracerName = new String[] {"none"};
        String[] initializationToneMapName = new String[] {"Lightness"};
        String[] renderToneMapName = new String[] {"Lightness"};
        int[] argcRef = new int[] {argc};
        mainParseOptions(
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
            stochasticRayTracingState);

        // 3. Load scene elements from MGF file
        mgfContext.radianceMethod = selectedRadianceMethod;
        mgfContext.parserConfig.radianceMethod = selectedRadianceMethod;
        mgfContext.monochrome = DEFAULT_MONOCHROME;
        mgfContext.parserConfig.monochrome = DEFAULT_MONOCHROME;
        mgfContext.currentMaterial = defaultMaterial;
        mgfContext.materialState.currentMaterial = defaultMaterial;
        selectToneMapByName(initializationToneMapName[0]); // Note this is used for basic Galerkin model initialization
        SceneBuilder.sceneBuilderCreateModel(argcRef, argv, mgfContext, scene, toneMapOptions);
        selectToneMapByName(renderToneMapName[0]);

        // 4. Run main radiosity simulation and export result
        executeRendering(
            rayTracerName[0],
            rayMatterState,
            bidirectionalPathState,
            stochasticRayTracingState,
            lightList);

        // X. Interactive visual debug GUI tool (JOGL + Swing)
        if ( glutDebugEnabled ) {
            GlutDebugState debugState = new GlutDebugState();
            GlutDebugToolsModel debugToolsModel = new GlutDebugToolsModel();
            debugToolsModel.scene = scene;
            debugToolsModel.radianceMethod = mgfContext.radianceMethod;
            debugToolsModel.renderOptions = renderOptions;
            debugToolsModel.toneMapOptions = toneMapOptions;
            debugToolsModel.debugState = debugState;
            debugToolsModel.memoryFreeCallBack = RpkApplication::freeMemory;
            debugToolsModel.mgfContext = mgfContext;

            GlutDebugTools glutDebugTools = new GlutDebugTools(debugToolsModel);
            glutDebugTools.executeGlutGui(argc, argv);
        }

        // 5. Free used memory
        freeMemory(mgfContext);

        if ( rayTracer != null ) {
            rayTracer.terminate();
            rayTracer = null;
        }
        if ( lightList[0] != null ) {
            lightList[0] = null;
        }

        return 0;
    }
}
