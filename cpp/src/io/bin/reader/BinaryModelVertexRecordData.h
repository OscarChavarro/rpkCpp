#ifndef BINARY_MODEL_READER_VERTEX_RECORD__
#define BINARY_MODEL_READER_VERTEX_RECORD__

#include "common/color/ColorRgb.h"
#include "io/bin/reader/BinaryModelIndexListRef.h"

class BinaryModelVertexRecordData {
  public:
    int id;
    int pointIndex;
    int normalIndex;
    int textureCoordinateIndex;
    ColorRgb color;
    int backIndex;
    int tmp;
    bool hasRadianceData;
    BinaryModelIndexListRef patchIndices;

    BinaryModelVertexRecordData();
};

#endif
