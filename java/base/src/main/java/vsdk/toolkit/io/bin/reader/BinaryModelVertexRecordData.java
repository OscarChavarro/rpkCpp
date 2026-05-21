package vsdk.toolkit.io.bin.reader;

import vsdk.toolkit.common.color.ColorRgbMutable;

public class BinaryModelVertexRecordData {
    public int id;
    public int pointIndex;
    public int normalIndex;
    public int textureCoordinateIndex;
    public ColorRgbMutable color;
    public int backIndex;
    public int tmp;
    public boolean hasRadianceData;
    public BinaryModelIndexListRef patchIndices;

    public BinaryModelVertexRecordData() {
        id = 0;
        pointIndex = -1;
        normalIndex = -1;
        textureCoordinateIndex = -1;
        color = new ColorRgbMutable();
        backIndex = -1;
        tmp = 0;
        hasRadianceData = false;
        patchIndices = new BinaryModelIndexListRef();
    }
}
