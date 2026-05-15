#ifndef BINARY_MODEL_READER_PATCH_RECORD__
#define BINARY_MODEL_READER_PATCH_RECORD__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class BinaryModelPatchRecordData {
  public:
    int id;
    int twinIndex;
    int numberOfVertices;
    int vertexIndices[MAXIMUM_VERTICES_PER_PATCH];
    bool hasBoundingBox;
    float boundingBoxCoordinates[6];
    Vector3D normal;
    float planeConstant;
    float tolerance;
    float area;
    Vector3D midPoint;
    bool hasJacobian;
    float jacobianA;
    float jacobianB;
    float jacobianC;
    float directPotential;
    int dominantIndex;
    bool omit;
    unsigned char flags;
    ColorRgb color;
    int materialIndex;
    bool hasRadianceData;

    BinaryModelPatchRecordData();
};

#endif
