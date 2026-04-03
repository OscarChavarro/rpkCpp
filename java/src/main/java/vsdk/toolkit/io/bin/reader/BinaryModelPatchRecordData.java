package vsdk.toolkit.io.bin.reader;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.Patch;

public class BinaryModelPatchRecordData {
    public int id;
    public int twinIndex;
    public int numberOfVertices;
    public int[] vertexIndices;
    public boolean hasBoundingBox;
    public float[] boundingBoxCoordinates;
    public Vector3D normal;
    public float planeConstant;
    public float tolerance;
    public float area;
    public Vector3D midPoint;
    public boolean hasJacobian;
    public float jacobianA;
    public float jacobianB;
    public float jacobianC;
    public float directPotential;
    public int dominantIndex;
    public boolean omit;
    public byte flags;
    public ColorRgb color;
    public int materialIndex;
    public boolean hasRadianceData;

    public BinaryModelPatchRecordData() {
        id = 0;
        twinIndex = -1;
        numberOfVertices = 0;
        vertexIndices = new int[Patch.MAXIMUM_VERTICES_PER_PATCH];
        for (int i = 0; i < Patch.MAXIMUM_VERTICES_PER_PATCH; i++) {
            vertexIndices[i] = -1;
        }
        hasBoundingBox = false;
        boundingBoxCoordinates = new float[6];
        for (int i = 0; i < 6; i++) {
            boundingBoxCoordinates[i] = 0.0f;
        }
        normal = new Vector3D();
        planeConstant = 0.0f;
        tolerance = 0.0f;
        area = 0.0f;
        midPoint = new Vector3D();
        hasJacobian = false;
        jacobianA = 0.0f;
        jacobianB = 0.0f;
        jacobianC = 0.0f;
        directPotential = 0.0f;
        dominantIndex = 0;
        omit = false;
        flags = 0;
        color = new ColorRgb();
        materialIndex = -1;
        hasRadianceData = false;
    }
}
