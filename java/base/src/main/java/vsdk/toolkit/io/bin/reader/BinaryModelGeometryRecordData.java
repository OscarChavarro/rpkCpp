package vsdk.toolkit.io.bin.reader;

public class BinaryModelGeometryRecordData {
    public int classId;
    public int id;
    public int itemCount;
    public boolean bounded;
    public boolean shaftCullGeometry;
    public boolean omit;
    public boolean isDuplicate;
    public float[] boundingBoxCoordinates;
    public boolean hasRayIntersectionBox;
    public boolean hasRadianceData;

    public boolean hasObjectName;
    public String objectName;
    public int meshId;
    public int materialIndex;
    public BinaryModelIndexListRef positions;
    public BinaryModelIndexListRef normals;
    public BinaryModelIndexListRef vertices;
    public BinaryModelIndexListRef faces;

    public BinaryModelIndexListRef children;
    public BinaryModelIndexListRef patchSetPatches;

    public BinaryModelGeometryRecordData() {
        classId = 0;
        id = 0;
        itemCount = 0;
        bounded = false;
        shaftCullGeometry = false;
        omit = false;
        isDuplicate = false;
        boundingBoxCoordinates = new float[6];
        for (int i = 0; i < 6; i++) {
            boundingBoxCoordinates[i] = 0.0f;
        }
        hasRayIntersectionBox = false;
        hasRadianceData = false;

        hasObjectName = false;
        objectName = null;
        meshId = 0;
        materialIndex = -1;
        positions = new BinaryModelIndexListRef();
        normals = new BinaryModelIndexListRef();
        vertices = new BinaryModelIndexListRef();
        faces = new BinaryModelIndexListRef();

        children = new BinaryModelIndexListRef();
        patchSetPatches = new BinaryModelIndexListRef();
    }
}
