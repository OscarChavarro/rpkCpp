#ifndef __BINARY_MODEL_READER_MODEL_RECORD__
#define __BINARY_MODEL_READER_MODEL_RECORD__

#include "io/bin/BinaryModelReader.h"
#include "io/bin/BinaryModelReaderIndexListRecord.h"

class BinaryModelReader::ModelRecord {
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

    IndexListRecord currentFaceList;
    IndexListRecord currentGeometryList;
    IndexListRecord currentNormalList;
    IndexListRecord currentPointList;
    IndexListRecord currentVertexList;
    IndexListRecord geometries;
    IndexListRecord materials;

    ModelRecord():
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
        transformContextIndex(0)
    {
    }
};

#endif
