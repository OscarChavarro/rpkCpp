#ifndef __BINARY_MODEL_READER_PATCH_RECORD__
#define __BINARY_MODEL_READER_PATCH_RECORD__

#include "common/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"

#include "skin/Patch.h"

#include "io/bin/BinaryModelReader.h"

class BinaryModelReader::PatchRecord {
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
};

#endif
