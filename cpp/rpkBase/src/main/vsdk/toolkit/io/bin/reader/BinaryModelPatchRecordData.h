#ifndef __BINARY_MODEL_PATCH_RECORD_DATA__
#define __BINARY_MODEL_PATCH_RECORD_DATA__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class BinaryModelPatchRecordData {
  public:
    int id;
    int twinIndex;
    int numberOfVertices;
    int vertexIndices[MAXIMUM_VERTICES_PER_PATCH];
    bool hasBoundingBox;
    double boundingBoxCoordinates[6];
    Vector3Dd normal;
    double planeConstant;
    double tolerance;
    double area;
    Vector3Dd midPoint;
    bool hasJacobian;
    double jacobianA;
    double jacobianB;
    double jacobianC;
    double directPotential;
    int dominantIndex;
    bool omit;
    unsigned char flags;
    ColorRgb color;
    int materialIndex;
    bool hasRadianceData;

    BinaryModelPatchRecordData();
};

#endif
