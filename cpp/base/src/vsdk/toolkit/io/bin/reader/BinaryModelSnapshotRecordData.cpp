#include "vsdk/toolkit/io/bin/reader/BinaryModelSnapshotRecordData.h"

BinaryModelSnapshotRecordData::BinaryModelSnapshotRecordData():
    currentColorIndex(0),
    hasCurrentMaterialName(false),
    currentMaterialName(nullptr),
    hasCurrentObjectName(false),
    currentObjectName(nullptr),
    hasCurrentVertexName(false),
    currentVertexName(nullptr),
    geometryStackHeadIndex(0),
    inComplex(false),
    inSurface(false),
    monochrome(false),
    readerContextIndex(0),
    transformContextIndex(0),
    currentFaceList(),
    currentGeometryList(),
    currentNormalList(),
    currentPointList(),
    currentVertexList(),
    geometries(),
    materials()
{
}
