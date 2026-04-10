#include "io/context/ParseSnapshotContext.h"

ParseSnapshotContext::ParseSnapshotContext():
    currentColor(NULL),
    currentFaceList(NULL),
    currentGeometryList(NULL),
    currentMaterialName(NULL),
    currentNormalList(NULL),
    currentObjectName(NULL),
    currentPointList(NULL),
    currentVertexList(NULL),
    currentVertexName(NULL),
    geometries(NULL),
    geometryStackHeadIndex(0),
    inComplex(false),
    inSurface(false),
    materials(NULL),
    monochrome(false),
    readerContext(NULL),
    transformContext(NULL)
{
}
