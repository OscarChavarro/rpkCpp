#ifndef BINARY_MODEL_READER_MODEL_RECORD__
#define BINARY_MODEL_READER_MODEL_RECORD__

#include "io/bin/reader/BinaryModelIndexListRef.h"

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
