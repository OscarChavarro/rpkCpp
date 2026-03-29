#ifndef __BINARY_MODEL_READER_MODEL_RECORD__
#define __BINARY_MODEL_READER_MODEL_RECORD__

#include "io/bin/BinaryModelReaderIndexListRecord.h"

class BinaryModelReaderModelRecord {
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

    BinaryModelReaderIndexListRecord currentFaceList;
    BinaryModelReaderIndexListRecord currentGeometryList;
    BinaryModelReaderIndexListRecord currentNormalList;
    BinaryModelReaderIndexListRecord currentPointList;
    BinaryModelReaderIndexListRecord currentVertexList;
    BinaryModelReaderIndexListRecord geometries;
    BinaryModelReaderIndexListRecord materials;

    BinaryModelReaderModelRecord();
};

#endif
