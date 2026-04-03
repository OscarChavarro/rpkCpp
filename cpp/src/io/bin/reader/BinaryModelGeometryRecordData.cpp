#include "io/bin/reader/BinaryModelGeometryRecordData.h"

BinaryModelGeometryRecordData::BinaryModelGeometryRecordData():
    classId(0),
    id(0),
    itemCount(0),
    bounded(false),
    shaftCullGeometry(false),
    omit(false),
    isDuplicate(false),
    hasRayIntersectionBox(false),
    hasRadianceData(false),
    hasObjectName(false),
    objectName(nullptr),
    meshId(0),
    materialIndex(-1),
    positions(),
    normals(),
    vertices(),
    faces(),
    children(),
    patchSetPatches()
{
    for ( int i = 0; i < 6; i++ ) {
        boundingBoxCoordinates[i] = 0.0f;
    }
}
