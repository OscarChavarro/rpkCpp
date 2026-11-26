#include "vsdk/toolkit/io/bin/reader/BinaryModelPatchRecordData.h"

BinaryModelPatchRecordData::BinaryModelPatchRecordData():
    id(0),
    twinIndex(-1),
    numberOfVertices(0),
    hasBoundingBox(false),
    normal(),
    planeConstant(0.0),
    tolerance(0.0),
    area(0.0),
    midPoint(),
    hasJacobian(false),
    jacobianA(0.0),
    jacobianB(0.0),
    jacobianC(0.0),
    directPotential(0.0),
    dominantIndex(0),
    omit(false),
    flags(0),
    color(0.0, 0.0, 0.0),
    materialIndex(-1),
    hasRadianceData(false)
{
    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        vertexIndices[i] = -1;
    }
    for ( int i = 0; i < 6; i++ ) {
        boundingBoxCoordinates[i] = 0.0;
    }
}
