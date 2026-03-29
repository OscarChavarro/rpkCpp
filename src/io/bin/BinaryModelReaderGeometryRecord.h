#ifndef __BINARY_MODEL_READER_GEOMETRY_RECORD__
#define __BINARY_MODEL_READER_GEOMETRY_RECORD__

#include "io/bin/BinaryModelReaderIndexListRecord.h"

class BinaryModelReaderGeometryRecord {
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
    BinaryModelReaderIndexListRecord positions;
    BinaryModelReaderIndexListRecord normals;
    BinaryModelReaderIndexListRecord vertices;
    BinaryModelReaderIndexListRecord faces;

    BinaryModelReaderIndexListRecord children;
    BinaryModelReaderIndexListRecord patchSetPatches;

    BinaryModelReaderGeometryRecord();
};

#endif
