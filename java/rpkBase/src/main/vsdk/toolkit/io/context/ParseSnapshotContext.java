package vsdk.toolkit.io.context;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkVertex;

public class ParseSnapshotContext {
    // Snapshot of MGF parsing state and outputs (non-owning references).
    public ColorContext currentColor;
    public ArrayList<Patch> currentFaceList;
    public ArrayList<RpkGeometry> currentGeometryList;
    public String currentMaterialName;
    public ArrayList<Vector3Dd> currentNormalList;
    public String currentObjectName;
    public ArrayList<Vector3Dd> currentPointList;
    public ArrayList<RpkVertex> currentVertexList;
    public String currentVertexName;
    public ArrayList<RpkGeometry> geometries;
    public int geometryStackHeadIndex;
    public boolean inComplex;
    public boolean inSurface;
    public ArrayList<RpkMaterial> materials;
    public boolean monochrome;
    public boolean singleSided;
    public int numberOfQuarterCircleDivisions;
    public ReaderContext readerContext;
    public TransformStackContext transformContext;

    public ParseSnapshotContext() {
        currentColor = null;
        currentFaceList = null;
        currentGeometryList = null;
        currentMaterialName = null;
        currentNormalList = null;
        currentObjectName = null;
        currentPointList = null;
        currentVertexList = null;
        currentVertexName = null;
        geometries = null;
        geometryStackHeadIndex = 0;
        inComplex = false;
        inSurface = false;
        materials = null;
        monochrome = false;
        singleSided = false;
        numberOfQuarterCircleDivisions = 0;
        readerContext = null;
        transformContext = null;
    }
}
