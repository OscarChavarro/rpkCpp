package vsdk.toolkit.io.context;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.material.RpkMaterial;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.Vertex;

public class ParseSnapshotContext {
    // Snapshot of MGF parsing state and outputs (non-owning references).
    public ColorContext currentColor;
    public ArrayList<Patch> currentFaceList;
    public ArrayList<Geometry> currentGeometryList;
    public String currentMaterialName;
    public ArrayList<Vector3Df> currentNormalList;
    public String currentObjectName;
    public ArrayList<Vector3Df> currentPointList;
    public ArrayList<Vertex> currentVertexList;
    public String currentVertexName;
    public ArrayList<Geometry> geometries;
    public int geometryStackHeadIndex;
    public boolean inComplex;
    public boolean inSurface;
    public ArrayList<RpkMaterial> materials;
    public boolean monochrome;
    public boolean singleSided;
    public boolean warpConeEnds;
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
        warpConeEnds = false;
        numberOfQuarterCircleDivisions = 0;
        readerContext = null;
        transformContext = null;
    }
}
