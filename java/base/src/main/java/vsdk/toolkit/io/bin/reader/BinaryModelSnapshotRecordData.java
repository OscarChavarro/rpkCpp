package vsdk.toolkit.io.bin.reader;

public class BinaryModelSnapshotRecordData {
    public int currentColorIndex;
    public boolean hasCurrentMaterialName;
    public String currentMaterialName;
    public boolean hasCurrentObjectName;
    public String currentObjectName;
    public boolean hasCurrentVertexName;
    public String currentVertexName;
    public int geometryStackHeadIndex;
    public boolean inComplex;
    public boolean inSurface;
    public boolean monochrome;
    public boolean singleSided;
    public boolean warpConeEnds;
    public int numberOfQuarterCircleDivisions;
    public int readerContextIndex;
    public int transformContextIndex;

    public BinaryModelIndexListRef currentFaceList;
    public BinaryModelIndexListRef currentGeometryList;
    public BinaryModelIndexListRef currentNormalList;
    public BinaryModelIndexListRef currentPointList;
    public BinaryModelIndexListRef currentVertexList;
    public BinaryModelIndexListRef geometries;
    public BinaryModelIndexListRef materials;

    public BinaryModelSnapshotRecordData() {
        currentColorIndex = 0;
        hasCurrentMaterialName = false;
        currentMaterialName = null;
        hasCurrentObjectName = false;
        currentObjectName = null;
        hasCurrentVertexName = false;
        currentVertexName = null;
        geometryStackHeadIndex = 0;
        inComplex = false;
        inSurface = false;
        monochrome = false;
        singleSided = false;
        warpConeEnds = false;
        numberOfQuarterCircleDivisions = 0;
        readerContextIndex = 0;
        transformContextIndex = 0;
        currentFaceList = new BinaryModelIndexListRef();
        currentGeometryList = new BinaryModelIndexListRef();
        currentNormalList = new BinaryModelIndexListRef();
        currentPointList = new BinaryModelIndexListRef();
        currentVertexList = new BinaryModelIndexListRef();
        geometries = new BinaryModelIndexListRef();
        materials = new BinaryModelIndexListRef();
    }
}
