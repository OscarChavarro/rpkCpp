#include "vsdk/toolkit/io/bin/reader/BinaryModelVertexRecordData.h"

BinaryModelVertexRecordData::BinaryModelVertexRecordData():
    id(0),
    pointIndex(-1),
    normalIndex(-1),
    textureCoordinateIndex(-1),
    color(0.0, 0.0, 0.0),
    backIndex(-1),
    tmp(0),
    hasRadianceData(false),
    patchIndices()
{
}
