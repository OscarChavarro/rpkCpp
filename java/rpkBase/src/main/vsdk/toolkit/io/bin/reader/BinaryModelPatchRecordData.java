package vsdk.toolkit.io.bin.reader;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.elements.Patch;

public class BinaryModelPatchRecordData {
    public int id;
    public int twinIndex;
    public int numberOfVertices;
    public int[] vertexIndices;
    public boolean hasBoundingBox;
    public double[] boundingBoxCoordinates;
    public Vector3Dd normal;
    public double planeConstant;
    public double tolerance;
    public double area;
    public Vector3Dd midPoint;
    public boolean hasJacobian;
    public double jacobianA;
    public double jacobianB;
    public double jacobianC;
    public double directPotential;
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
        boundingBoxCoordinates = new double[6];
        for (int i = 0; i < 6; i++) {
            boundingBoxCoordinates[i] = 0.0;
        }
        normal = new Vector3Dd();
        planeConstant = 0.0;
        tolerance = 0.0;
        area = 0.0;
        midPoint = new Vector3Dd();
        hasJacobian = false;
        jacobianA = 0.0;
        jacobianB = 0.0;
        jacobianC = 0.0;
        directPotential = 0.0;
        dominantIndex = 0;
        omit = false;
        flags = 0;
        color = new ColorRgb();
        materialIndex = -1;
        hasRadianceData = false;
    }
}
