package vsdk.toolkit.io.context;

import java.util.ArrayList;
import vsdk.toolkit.common.dataStructures.LookUpTable;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;

public final class ParseRuntimeContext extends ParseContext {
    public ParseOptionsContext parserConfig;
    public ReaderDispatchContext readerStackState;
    public GeometryAssemblyContext geometryBuildState;
    public MaterialSelectionContext materialState;
    public ColorRegistryContext colorRepository;
    public MaterialRegistryContext materialRepository;
    public VertexRegistryContext vertexRepository;
    public ObjectScopeContext objectHierarchyState;
    public TransformScopeContext transformStack;

    public ParseSnapshotContext model;

    // Transitional aliases to keep current code building while the call sites
    // migrate to explicit sub-state access.
    public RadianceMethod radianceMethod;
    public boolean singleSided;
    public String currentVertexName;
    public int numberOfQuarterCircleDivisions;
    public boolean monochrome;
    public RpkMaterial currentMaterial;
    public String[] entityNames;
    public String[] errorCodeMessages;
    public LookUpTable<String> entityLookUpTable;
    public int nextFileContextId;
    public ReaderContext readerContext;
    public String currentMaterialName;
    public int geometryStackHeadIndex;
    public ArrayList<RpkGeometry>[] geometryStack;
    public ArrayList<Vector3Dd> currentPointList;
    public ArrayList<Vector3Dd> currentNormalList;
    public ArrayList<RpkVertex> currentVertexList;
    public ArrayList<Patch> currentFaceList;
    public ArrayList<RpkGeometry> currentGeometryList;
    public String currentObjectName;
    public TransformStackContext transformContext;
    public ColorContext unNamedColorContext;
    public ColorContext currentColor;
    public boolean inSurface;
    public boolean inComplex;
    public LookUpTable<VertexContext> vertexLookUpTable;
    public ArrayList<RpkGeometry> allGeometries;
    public ArrayList<RpkGeometry> geometries;
    public ArrayList<RpkMaterial> materials;

    public ParseRuntimeContext() {
        parserConfig = new ParseOptionsContext();
        readerStackState = new ReaderDispatchContext();
        geometryBuildState = new GeometryAssemblyContext();
        materialState = new MaterialSelectionContext();
        colorRepository = new ColorRegistryContext();
        materialRepository = new MaterialRegistryContext();
        vertexRepository = new VertexRegistryContext();
        objectHierarchyState = new ObjectScopeContext();
        transformStack = new TransformScopeContext();
        model = null;

        radianceMethod = parserConfig.radianceMethod;
        singleSided = parserConfig.singleSided;
        currentVertexName = geometryBuildState.currentVertexName;
        numberOfQuarterCircleDivisions = parserConfig.numberOfQuarterCircleDivisions;
        monochrome = parserConfig.monochrome;
        currentMaterial = materialState.currentMaterial;
        entityNames = readerStackState.entityNames;
        errorCodeMessages = readerStackState.errorCodeMessages;
        entityLookUpTable = readerStackState.entityLookUpTable;
        nextFileContextId = readerStackState.nextFileContextId;
        readerContext = readerStackState.readerContext;
        currentMaterialName = materialState.currentMaterialName;
        geometryStackHeadIndex = geometryBuildState.geometryStackHeadIndex;
        geometryStack = geometryBuildState.geometryStack;
        currentPointList = geometryBuildState.currentPointList;
        currentNormalList = geometryBuildState.currentNormalList;
        currentVertexList = geometryBuildState.currentVertexList;
        currentFaceList = geometryBuildState.currentFaceList;
        currentGeometryList = geometryBuildState.currentGeometryList;
        currentObjectName = geometryBuildState.currentObjectName;
        transformContext = transformStack.transformContext;
        unNamedColorContext = colorRepository.unNamedColorContext;
        currentColor = colorRepository.currentColor;
        inSurface = geometryBuildState.inSurface;
        inComplex = geometryBuildState.inComplex;
        vertexLookUpTable = vertexRepository.vertexLookUpTable;
        allGeometries = geometryBuildState.allGeometries;
        geometries = geometryBuildState.geometries;
        materials = materialState.materials;
    }

    public void destroy() {
        if (model != null) {
            model = null;
        }
    }
}
