#include "io/context/ParseSnapshotContext.h"

ParseSnapshotContext::ParseSnapshotContext():
    currentColor(nullptr),
    currentFaceList(nullptr),
    currentGeometryList(nullptr),
    currentMaterialName(nullptr),
    currentNormalList(nullptr),
    currentObjectName(nullptr),
    currentPointList(nullptr),
    currentVertexList(nullptr),
    currentVertexName(nullptr),
    geometries(nullptr),
    geometryStackHeadIndex(0),
    inComplex(false),
    inSurface(false),
    materials(nullptr),
    monochrome(false),
    readerContext(nullptr),
    transformContext(nullptr)
{
}
