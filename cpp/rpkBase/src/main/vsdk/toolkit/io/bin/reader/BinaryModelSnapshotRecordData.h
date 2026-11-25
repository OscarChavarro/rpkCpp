#ifndef __BINARY_MODEL_SNAPSHOT_RECORD_DATA__
#define __BINARY_MODEL_SNAPSHOT_RECORD_DATA__

#include "vsdk/toolkit/io/bin/reader/BinaryModelIndexListRef.h"

class BinaryModelSnapshotRecordData {
  public:
    int currentColorIndex;
    bool hasCurrentMaterialName;
    char *currentMaterialName;
    bool hasCurrentObjectName;
    char *currentObjectName;
    bool hasCurrentVertexName;
    char *currentVertexName;
    int geometryStackHeadIndex;
    bool inComplex;
    bool inSurface;
    bool monochrome;
    int readerContextIndex;
    int transformContextIndex;

    BinaryModelIndexListRef currentFaceList;
    BinaryModelIndexListRef currentGeometryList;
    BinaryModelIndexListRef currentNormalList;
    BinaryModelIndexListRef currentPointList;
    BinaryModelIndexListRef currentVertexList;
    BinaryModelIndexListRef geometries;
    BinaryModelIndexListRef materials;

    BinaryModelSnapshotRecordData();
};

#endif
