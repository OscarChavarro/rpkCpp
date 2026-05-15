#include "io/bin/reader/BinaryModelPatchRecordData.h"

BinaryModelPatchRecordData::BinaryModelPatchRecordData():
    id(0),
    twinIndex(-1),
    numberOfVertices(0),
    hasBoundingBox(false),
    normal(),
    planeConstant(0.0f),
    tolerance(0.0f),
    area(0.0f),
    midPoint(),
    hasJacobian(false),
    jacobianA(0.0f),
    jacobianB(0.0f),
    jacobianC(0.0f),
    directPotential(0.0f),
    dominantIndex(0),
    omit(false),
    flags(0),
    color(),
    materialIndex(-1),
    hasRadianceData(false)
{
    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        vertexIndices[i] = -1;
    }
    for ( int i = 0; i < 6; i++ ) {
        boundingBoxCoordinates[i] = 0.0f;
    }
}
