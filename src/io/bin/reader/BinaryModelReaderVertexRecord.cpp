#include "io/bin/reader/BinaryModelReaderVertexRecord.h"

BinaryModelReaderVertexRecord::BinaryModelReaderVertexRecord():
    id(0),
    pointIndex(-1),
    normalIndex(-1),
    textureCoordinateIndex(-1),
    color(),
    backIndex(-1),
    tmp(0),
    hasRadianceData(false),
    patchIndices()
{
}
