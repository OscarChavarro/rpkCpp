#include "vsdk/toolkit/io/bin/reader/BinaryModelPatchRecordData.h"

BinaryModelPatchRecordData::BinaryModelPatchRecordData():
    id(0),
    twinIndex(-1),
    numberOfVertices(0),
    hasBoundingBox(false),
    normal(),
    planeConstant(0.0F),
    tolerance(0.0F),
    area(0.0F),
    midPoint(),
    hasJacobian(false),
    jacobianA(0.0F),
    jacobianB(0.0F),
    jacobianC(0.0F),
    directPotential(0.0F),
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
        boundingBoxCoordinates[i] = 0.0F;
    }
}
