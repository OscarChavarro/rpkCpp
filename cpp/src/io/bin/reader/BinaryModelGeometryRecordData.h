#ifndef __BINARY_MODEL_READER_GEOMETRY_RECORD__
#define __BINARY_MODEL_READER_GEOMETRY_RECORD__

#include "io/bin/reader/BinaryModelIndexListRef.h"

class BinaryModelGeometryRecordData {
  public:
    int classId;
    int id;
    int itemCount;
    bool bounded;
    bool shaftCullGeometry;
    bool omit;
    bool isDuplicate;
    float boundingBoxCoordinates[6];
    bool hasRayIntersectionBox;
    bool hasRadianceData;

    bool hasObjectName;
    char *objectName;
    int meshId;
    int materialIndex;
    BinaryModelIndexListRef positions;
    BinaryModelIndexListRef normals;
    BinaryModelIndexListRef vertices;
    BinaryModelIndexListRef faces;

    BinaryModelIndexListRef children;
    BinaryModelIndexListRef patchSetPatches;

    BinaryModelGeometryRecordData();
};

#endif
